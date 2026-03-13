use crate::ast::{Grammar, Op, Rule};
use rand::Rng;
use std::io;

pub struct Generator<'a, R: Rng> {
    grammar: &'a Grammar,
    rng: &'a mut R,
    max_depth: usize,
    current_class: Vec<String>,
}

impl<'a, R: Rng> Generator<'a, R> {
    pub fn new(grammar: &'a Grammar, rng: &'a mut R, max_depth: usize) -> Self {
        Generator {
            grammar,
            rng,
            max_depth,
            current_class: Vec::new(),
        }
    }

    pub fn generate_ast(&mut self) -> io::Result<Rule> {
        let entry_point = self
            .grammar
            .entry_points
            .get(0)
            .ok_or_else(|| io::Error::new(io::ErrorKind::NotFound, "No entry points found"))?
            .clone();

        let entry_point = self
            .grammar
            .rules
            .get(&entry_point)
            .ok_or_else(|| io::Error::new(io::ErrorKind::NotFound, "Entry point rule not found"))?
            .clone();

        let mut generated_ops = Vec::new();
        let op = self.generate_op(&entry_point.definition, 0)?;
        if op != Op::Empty {
            generated_ops.push(op);
        } else {
            let fallback = self.generate_op(&Op::RuleRef("topLevel".to_string()), 0)?;
            if fallback != Op::Empty {
                generated_ops.push(fallback);
            }
        }
        generated_ops.push(Op::Terminal(self.generate_main_function()));

        let op = if generated_ops.len() == 1 {
            generated_ops.pop().unwrap()
        } else {
            Op::Sequence(generated_ops)
        };
        Ok(Rule {
            name: entry_point.name,
            rule_type: entry_point.rule_type,
            definition: op,
            skip: entry_point.skip,
        })
    }

    pub fn generate_op(&mut self, op: &Op, depth: usize) -> io::Result<Op> {
        if depth >= self.max_depth {
            if let Some(fallback) = self.generate_depth_limited_op(op)? {
                return Ok(fallback);
            }

            if !op.is_trivial_rule() {
                return Ok(Op::Empty);
            }
        }

        match op {
            Op::Terminal(s) => match s.as_str() {
                "" => Ok(Op::Empty),
                _ => Ok(Op::Terminal(s.clone())),
            },
            Op::RuleRef(_) => self.generate_rule_ref(op, depth),
            Op::ZeroOrMore(op) => self.generate_zero_or_more(op, depth),
            Op::OneOrMore(op) => self.generate_one_or_more(op, depth),
            Op::Sequence(ops) => self.generate_sequence(ops, depth),
            Op::Alternative(ops) => self.generate_alternative(ops, depth),
            Op::CharRange(start, end) => Ok(Op::Terminal(
                self.generate_char_in_range(*start, *end).to_string(),
            )),
            Op::Set(ops) => self.generate_set(ops, depth),
            Op::Optional(op) => self.generate_optional(op, depth),
            Op::Not(op) => self.generate_not(op, depth),
            Op::Parentheses(op) => {
                let inner_op = self.generate_op(op, depth)?;
                match inner_op {
                    Op::Empty => Ok(Op::Empty),
                    _ => Ok(Op::Parentheses(Box::new(inner_op))),
                }
            }
            Op::AnyChar => {
                let generated_char = self.generate_char_in_range('\u{0000}', '\u{FFFF}');
                Ok(Op::Terminal(generated_char.to_string()))
            }
            Op::Empty => Ok(Op::Empty),
            Op::Skip => Ok(Op::Skip),
            Op::EofDefinition => Ok(Op::EofDefinition),
        }
    }

    pub fn stringify_op(&mut self, op: &Op) -> String {
        match op {
            Op::Terminal(s) => s.clone(),
            Op::Sequence(ops) => {
                let mut result = String::new();
                for (_, op) in ops.iter().enumerate() {
                    let s = self.stringify_op(op);
                    if Self::needs_separator(result.as_str(), s.as_str()) {
                        result.push(' ');
                    }
                    result.push_str(&s);
                }
                result
            }
            Op::ZeroOrMore(op) => self.stringify_op(op),
            Op::OneOrMore(op) => self.stringify_op(op),
            Op::Parentheses(op) => self.stringify_op(op),
            Op::Optional(op) => self.stringify_op(op),
            _ => String::new(),
        }
    }

    fn needs_separator(left: &str, right: &str) -> bool {
        let Some(left_char) = left.chars().next_back() else {
            return false;
        };
        let Some(right_char) = right.chars().next() else {
            return false;
        };

        Self::is_word_char(left_char) && Self::is_word_char(right_char)
    }

    fn is_word_char(c: char) -> bool {
        c.is_ascii_alphanumeric() || c == '_'
    }

    fn generate_set(&mut self, ops: &[Op], depth: usize) -> io::Result<Op> {
        let Some(chosen) = ops.get(self.rng.random_range(0..ops.len())) else {
            return Ok(Op::Empty);
        };

        match self.generate_op(chosen, depth + 1)? {
            Op::Terminal(c) => Ok(Op::Terminal(c)),
            Op::CharRange(start, end) => Ok(Op::Terminal(
                self.generate_char_in_range(start, end).to_string(),
            )),
            Op::Empty => Ok(Op::Empty),
            _ => Err(io::Error::new(
                io::ErrorKind::InvalidData,
                "Unsupported operation in set",
            )),
        }
    }

    fn generate_alternative(&mut self, ops: &[Op], depth: usize) -> io::Result<Op> {
        let mut generated_ops = Vec::new();
        for op in ops {
            let gen_op = self.generate_op(op, depth + 1)?;
            match gen_op {
                Op::Empty => continue,
                Op::Sequence(ref inner_ops) => {
                    if inner_ops.is_empty() {
                        continue;
                    } else {
                        generated_ops.push(gen_op);
                    }
                }
                _ => generated_ops.push(gen_op),
            }
        }
        if generated_ops.is_empty() {
            return Ok(Op::Empty);
        }

        let tk = self.rng.random_range(0..generated_ops.len());
        Ok(generated_ops[tk].clone())
    }

    fn generate_sequence(&mut self, ops: &[Op], depth: usize) -> io::Result<Op> {
        let mut generated_ops = Vec::new();
        for op in ops {
            let gen_op = self.generate_op(op, depth + 1)?;
            match gen_op {
                Op::Empty => continue,
                Op::Sequence(ref inner_ops) => {
                    if inner_ops.is_empty() {
                        continue;
                    } else {
                        generated_ops.push(gen_op);
                    }
                }
                _ => generated_ops.push(gen_op),
            }
        }
        if generated_ops.is_empty() {
            return Ok(Op::Empty);
        }
        if generated_ops.len() == 1 {
            return Ok(generated_ops.into_iter().next().unwrap());
        }
        Ok(Op::Sequence(generated_ops))
    }

    fn generate_one_or_more(&mut self, op: &Op, depth: usize) -> io::Result<Op> {
        const MAX: usize = 30;
        let mut v = Vec::new();
        for _i in 0..self.rng.random_range(1..MAX) {
            let inner_op = self.generate_op(op, depth)?;
            match inner_op {
                Op::Empty => continue,
                _ => v.push(inner_op),
            }
        }
        if v.is_empty() {
            return Ok(Op::Empty);
        }
        if v.len() == 1 {
            return Ok(v.into_iter().next().unwrap());
        }
        Ok(Op::Sequence(v))
    }

    fn generate_zero_or_more(&mut self, op: &Op, depth: usize) -> io::Result<Op> {
        const MAX: usize = 30;
        let prune = self.rng.random_ratio(1, 10);
        if prune {
            return Ok(Op::Empty);
        }
        let mut v = Vec::new();
        for _i in 0..self.rng.random_range(0..MAX) {
            let inner_op = self.generate_op(op, depth)?;
            match inner_op {
                Op::Empty => continue,
                _ => v.push(inner_op),
            }
        }
        if v.is_empty() {
            return Ok(Op::Empty);
        }
        if v.len() == 1 {
            return Ok(v.into_iter().next().unwrap());
        }
        Ok(Op::Sequence(v))
    }

    fn generate_char_in_range(&mut self, start: char, end: char) -> char {
        let range = end as u32 - start as u32 + 1;
        let random_offset = self.rng.random_range(0..range);
        let mut u = start as u32 + random_offset;
        const SURROGATE_START: u32 = 0xD800;
        const SURROGATE_END: u32 = 0xDFFF;
        while u >= SURROGATE_START && u <= SURROGATE_END {
            let random_offset = self.rng.random_range(0..range);
            u = start as u32 + random_offset;
        }
        let c =
            char::from_u32(u).expect(format!("Generated character is not valid: {}", u).as_str());
        c
    }

    fn generate_rule_ref(&mut self, op: &Op, depth: usize) -> io::Result<Op> {
        match op {
            Op::RuleRef(name) => {
                if name == "expression" {
                    return self.generate_expression(op, depth);
                }
                if name == "classDefinition" {
                    return Ok(Op::Terminal(self.generate_class_definition()));
                }
                if name == "constructorDefinition" && self.current_class_name().is_some() {
                    return Ok(Op::Terminal(self.generate_constructor_definition()));
                }
                if name == "functionDefinition" && self.current_class_name().is_some() {
                    return Ok(Op::Terminal(self.generate_member_function_definition()));
                }
                if name == "type" {
                    return Ok(Op::Terminal(self.generate_type()));
                }
                if name == "IDENTIFIER" {
                    return Ok(Op::Terminal(self.generate_identifier()));
                }
                if name == "parameter" {
                    return Ok(Op::Terminal(self.generate_parameter()));
                }
                if name == "parameterList" {
                    return Ok(Op::Terminal(self.generate_parameter_list(depth)));
                }
                if name == "expressionList" {
                    return Ok(Op::Terminal(self.generate_expression_list(depth)));
                }
                if name == "INTEGER" {
                    return Ok(Op::Terminal(self.generate_integer_literal()));
                }
                if name == "FLOAT" {
                    return Ok(Op::Terminal(self.generate_float_literal()));
                }
                if name == "STRING_LITERAL" {
                    return Ok(Op::Terminal(self.generate_string_literal()));
                }
                if name == "CHAR_LITERAL" {
                    return Ok(Op::Terminal(self.generate_char_literal()));
                }
                let sub_rule = self.grammar.rules.get(name).ok_or_else(|| {
                    io::Error::new(io::ErrorKind::NotFound, format!("Rule {} not found", name))
                })?;
                self.generate_op(&sub_rule.definition, depth + 1)
            }
            _ => Err(io::Error::new(
                io::ErrorKind::InvalidData,
                "Expected RuleRef",
            )),
        }
    }

    fn generate_identifier(&mut self) -> String {
        loop {
            let mut identifier = String::new();
            let len = self.rng.random_range(1..12);
            identifier.reserve(len);
            identifier.push(self.generate_ident_start_char());
            for _ in 1..len {
                identifier.push(self.generate_ident_continue_char());
            }

            if identifier != "main" && identifier != "self" {
                return identifier;
            }
        }
    }

    fn generate_expression(&mut self, op: &Op, depth: usize) -> io::Result<Op> {
        if depth + 1 >= self.max_depth || self.rng.random_ratio(1, 3) {
            return Ok(Op::Terminal(self.generate_simple_expression()));
        }

        self.generate_op(op, depth + 1)
    }

    fn generate_depth_limited_op(&mut self, op: &Op) -> io::Result<Option<Op>> {
        let fallback = match op {
            Op::RuleRef(name) => match name.as_str() {
                "program" => Some(Op::Empty),
                "topLevel" => Some(Op::Terminal(self.generate_function_definition())),
                "block" => Some(Op::Terminal("{}".to_string())),
                "statement" => Some(Op::Terminal(";".to_string())),
                "fullDeclaration" => Some(Op::Terminal(self.generate_full_declaration())),
                "declaration" => Some(Op::Terminal(self.generate_declaration())),
                "functionDefinition" => Some(Op::Terminal(self.generate_function_definition())),
                "classDefinition" => Some(Op::Terminal(self.generate_class_definition())),
                "typeAliasStatement" => Some(Op::Terminal(self.generate_type_alias_statement())),
                "ifStatement" => Some(Op::Terminal(format!(
                    "if ({}) {{}}",
                    self.generate_simple_expression()
                ))),
                "forStatement" => Some(Op::Terminal("for (;;) {}".to_string())),
                "parameter" => Some(Op::Terminal(self.generate_parameter())),
                "parameterList" => Some(Op::Terminal(String::new())),
                "expression" => Some(Op::Terminal(self.generate_simple_expression())),
                "expressionList" => {
                    Some(Op::Terminal(self.generate_expression_list(self.max_depth)))
                }
                "type" => Some(Op::Terminal(self.generate_type())),
                "STRING_LITERAL" => Some(Op::Terminal(self.generate_string_literal())),
                "CHAR_LITERAL" => Some(Op::Terminal(self.generate_char_literal())),
                "INTEGER" => Some(Op::Terminal(self.generate_integer_literal())),
                "FLOAT" => Some(Op::Terminal(self.generate_float_literal())),
                "IDENTIFIER" => Some(Op::Terminal(self.generate_identifier())),
                _ => None,
            },
            _ => None,
        };

        Ok(fallback)
    }

    fn generate_optional(&mut self, op: &Op, depth: usize) -> io::Result<Op> {
        if self.rng.random_ratio(1, 2) {
            Ok(Op::Empty)
        } else {
            self.generate_op(op, depth + 1)
        }
    }

    fn generate_not(&mut self, op: &Op, _depth: usize) -> io::Result<Op> {
        const ASCII_POOL: &[u8] =
            b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _";

        for _ in 0..32 {
            let candidate = ASCII_POOL[self.rng.random_range(0..ASCII_POOL.len())] as char;
            if !self.op_matches_char(op, candidate) {
                return Ok(Op::Terminal(candidate.to_string()));
            }
        }

        Ok(Op::Terminal("x".to_string()))
    }

    fn op_matches_char(&self, op: &Op, c: char) -> bool {
        match op {
            Op::Terminal(s) => s == &c.to_string(),
            Op::CharRange(start, end) => *start <= c && c <= *end,
            Op::Set(ops) | Op::Alternative(ops) | Op::Sequence(ops) => {
                ops.iter().any(|inner| self.op_matches_char(inner, c))
            }
            Op::Parentheses(inner) | Op::Optional(inner) | Op::OneOrMore(inner) => {
                self.op_matches_char(inner, c)
            }
            Op::ZeroOrMore(inner) => self.op_matches_char(inner, c),
            Op::Not(inner) => !self.op_matches_char(inner, c),
            _ => false,
        }
    }

    fn generate_ident_start_char(&mut self) -> char {
        const START: &[u8] = b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
        START[self.rng.random_range(0..START.len())] as char
    }

    fn generate_ident_continue_char(&mut self) -> char {
        const CONTINUE: &[u8] = b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
        CONTINUE[self.rng.random_range(0..CONTINUE.len())] as char
    }

    fn generate_integer_literal(&mut self) -> String {
        self.rng.random_range(0..10_000).to_string()
    }

    fn generate_float_literal(&mut self) -> String {
        format!(
            "{}.{}",
            self.rng.random_range(0..1_000),
            self.rng.random_range(0..1_000)
        )
    }

    fn generate_char_literal(&mut self) -> String {
        let c = self.generate_printable_char();
        format!("'{}'", self.escape_char(c))
    }

    fn generate_string_literal(&mut self) -> String {
        let len = self.rng.random_range(0..8);
        let mut s = String::from("\"");
        for _ in 0..len {
            let c = self.generate_printable_char();
            s.push_str(&self.escape_string_char(c));
        }
        s.push('"');
        s
    }

    fn generate_printable_char(&mut self) -> char {
        const PRINTABLE: &[u8] =
            b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-";
        PRINTABLE[self.rng.random_range(0..PRINTABLE.len())] as char
    }

    fn escape_char(&self, c: char) -> String {
        match c {
            '\\' => "\\\\".to_string(),
            '\'' => "\\'".to_string(),
            '\n' => "\\n".to_string(),
            '\r' => "\\r".to_string(),
            '\t' => "\\t".to_string(),
            _ => c.to_string(),
        }
    }

    fn escape_string_char(&self, c: char) -> String {
        match c {
            '\\' => "\\\\".to_string(),
            '"' => "\\\"".to_string(),
            '\n' => "\\n".to_string(),
            '\r' => "\\r".to_string(),
            '\t' => "\\t".to_string(),
            _ => c.to_string(),
        }
    }

    fn generate_type(&mut self) -> String {
        const TYPES: &[&str] = &[
            "bool", "char", "i8", "i16", "i32", "i64", "f32", "f64", "str",
        ];
        TYPES[self.rng.random_range(0..TYPES.len())].to_string()
    }

    fn current_class_name(&self) -> Option<&str> {
        self.current_class.last().map(String::as_str)
    }

    fn generate_simple_expression(&mut self) -> String {
        match self.rng.random_range(0..7) {
            0 => self.generate_integer_literal(),
            1 => self.generate_float_literal(),
            2 => "true".to_string(),
            3 => "false".to_string(),
            4 => self.generate_identifier(),
            5 => self.generate_char_literal(),
            _ => self.generate_string_literal(),
        }
    }

    fn generate_expression_list(&mut self, depth: usize) -> String {
        if depth + 1 >= self.max_depth || self.rng.random_ratio(1, 2) {
            return String::new();
        }

        let count = self.rng.random_range(1..=3);
        let mut expressions = Vec::with_capacity(count);
        for _ in 0..count {
            expressions.push(self.generate_simple_expression());
        }
        expressions.join(",")
    }

    fn generate_parameter(&mut self) -> String {
        format!("{}:{}", self.generate_identifier(), self.generate_type())
    }

    fn generate_parameter_list(&mut self, depth: usize) -> String {
        if depth + 1 >= self.max_depth || self.rng.random_ratio(1, 2) {
            return String::new();
        }

        let count = self.rng.random_range(1..=3);
        let mut parameters = Vec::with_capacity(count);
        for _ in 0..count {
            parameters.push(self.generate_parameter());
        }
        parameters.join(",")
    }

    fn generate_declaration(&mut self) -> String {
        format!(
            "var {}:{};",
            self.generate_identifier(),
            self.generate_type()
        )
    }

    fn generate_full_declaration(&mut self) -> String {
        format!(
            "var {}:{}={};",
            self.generate_identifier(),
            self.generate_type(),
            self.generate_simple_expression()
        )
    }

    fn generate_function_definition(&mut self) -> String {
        format!(
            "fn {}({}) {} {{}}",
            self.generate_identifier(),
            self.generate_parameter_list(self.max_depth),
            self.generate_type()
        )
    }

    fn generate_member_function_definition(&mut self) -> String {
        let class_name = self
            .current_class_name()
            .expect("member function generation requires class context")
            .to_string();
        let mut params = vec![format!("self:{}*", class_name)];
        let extra_params = self.generate_parameter_list(self.max_depth);
        if !extra_params.is_empty() {
            params.push(extra_params);
        }

        format!(
            "fn {}({}) {} {{}}",
            self.generate_identifier(),
            params.join(","),
            self.generate_type()
        )
    }

    fn generate_constructor_definition(&mut self) -> String {
        let class_name = self
            .current_class_name()
            .expect("constructor generation requires class context")
            .to_string();
        let mut params = vec![format!("self:{}*", class_name)];
        let extra_params = self.generate_parameter_list(self.max_depth);
        if !extra_params.is_empty() {
            params.push(extra_params);
        }

        format!("constructor ({}) {{}}", params.join(","))
    }

    fn generate_class_definition(&mut self) -> String {
        let class_name = self.generate_identifier();
        self.current_class.push(class_name.clone());

        let component_count = self.rng.random_range(0..=4);
        let mut components = Vec::with_capacity(component_count);
        for _ in 0..component_count {
            let component = match self.rng.random_range(0..3) {
                0 => self.generate_declaration(),
                1 => self.generate_member_function_definition(),
                _ => self.generate_constructor_definition(),
            };
            components.push(component);
        }

        self.current_class.pop();
        format!("class {}{{{}}}", class_name, components.join(""))
    }

    fn generate_type_alias_statement(&mut self) -> String {
        format!(
            "typealias {} {};",
            self.generate_type(),
            self.generate_identifier()
        )
    }

    fn generate_main_function(&mut self) -> String {
        "fn main() i32 {}".to_string()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ast::{Grammar, RuleType};
    use rand::{SeedableRng, rngs::StdRng};
    use std::collections::HashMap;

    fn minimal_grammar() -> Grammar {
        let mut rules = HashMap::new();
        rules.insert(
            "program".to_string(),
            Rule {
                name: "program".to_string(),
                rule_type: RuleType::Other,
                definition: Op::OneOrMore(Box::new(Op::RuleRef("topLevel".to_string()))),
                skip: false,
            },
        );
        rules.insert(
            "topLevel".to_string(),
            Rule {
                name: "topLevel".to_string(),
                rule_type: RuleType::Other,
                definition: Op::Alternative(vec![
                    Op::RuleRef("fullDeclaration".to_string()),
                    Op::RuleRef("functionDefinition".to_string()),
                ]),
                skip: false,
            },
        );
        rules.insert(
            "fullDeclaration".to_string(),
            Rule {
                name: "fullDeclaration".to_string(),
                rule_type: RuleType::Other,
                definition: Op::Sequence(vec![
                    Op::Terminal("var".to_string()),
                    Op::RuleRef("IDENTIFIER".to_string()),
                    Op::Terminal(":".to_string()),
                    Op::RuleRef("type".to_string()),
                    Op::Terminal("=".to_string()),
                    Op::RuleRef("expression".to_string()),
                    Op::Terminal(";".to_string()),
                ]),
                skip: false,
            },
        );
        rules.insert(
            "functionDefinition".to_string(),
            Rule {
                name: "functionDefinition".to_string(),
                rule_type: RuleType::Other,
                definition: Op::Sequence(vec![
                    Op::Terminal("fn".to_string()),
                    Op::RuleRef("IDENTIFIER".to_string()),
                    Op::Terminal("(".to_string()),
                    Op::RuleRef("parameterList".to_string()),
                    Op::Terminal(")".to_string()),
                    Op::RuleRef("type".to_string()),
                    Op::Terminal("{".to_string()),
                    Op::Terminal("}".to_string()),
                ]),
                skip: false,
            },
        );

        Grammar {
            rules,
            entry_points: vec!["program".to_string()],
        }
    }

    #[test]
    fn set_generation_emits_one_character() {
        let grammar = minimal_grammar();
        let mut rng = StdRng::seed_from_u64(7);
        let mut generator = Generator::new(&grammar, &mut rng, 4);

        let op = generator
            .generate_set(
                &[
                    Op::CharRange('a', 'z'),
                    Op::CharRange('A', 'Z'),
                    Op::Terminal("_".to_string()),
                ],
                0,
            )
            .unwrap();

        match op {
            Op::Terminal(s) => debug_assert_eq!(s.chars().count(), 1),
            other => panic!("expected terminal, got {:?}", other),
        }
    }

    #[test]
    fn depth_limited_declaration_keeps_required_syntax() {
        let grammar = minimal_grammar();
        let mut rng = StdRng::seed_from_u64(11);
        let mut generator = Generator::new(&grammar, &mut rng, 1);

        let generated = generator
            .generate_op(&Op::RuleRef("fullDeclaration".to_string()), 1)
            .unwrap();
        let rendered = generator.stringify_op(&generated);

        assert!(!rendered.contains(":;"));
        assert!(!rendered.contains("=;"));
        assert!(rendered.starts_with("var "));
    }

    #[test]
    fn generated_literals_are_well_formed() {
        let grammar = minimal_grammar();
        let mut rng = StdRng::seed_from_u64(19);
        let mut generator = Generator::new(&grammar, &mut rng, 4);

        let string_literal = generator.generate_string_literal();
        let char_literal = generator.generate_char_literal();

        assert!(string_literal.starts_with('"') && string_literal.ends_with('"'));
        assert!(char_literal.starts_with('\'') && char_literal.ends_with('\''));
    }

    #[test]
    fn stringify_separates_adjacent_word_tokens() {
        let grammar = minimal_grammar();
        let mut rng = StdRng::seed_from_u64(23);
        let mut generator = Generator::new(&grammar, &mut rng, 4);

        let rendered = generator.stringify_op(&Op::Sequence(vec![
            Op::Terminal("typealias".to_string()),
            Op::Terminal("char".to_string()),
            Op::Terminal("Bz".to_string()),
            Op::Terminal(";".to_string()),
        ]));

        assert_eq!(rendered, "typealias char Bz;");
    }

    #[test]
    fn class_generation_includes_self_in_constructors_and_methods() {
        let grammar = minimal_grammar();
        let mut rng = StdRng::seed_from_u64(29);
        let mut generator = Generator::new(&grammar, &mut rng, 4);

        generator.current_class.push("Widget".to_string());
        let constructor = generator.generate_constructor_definition();
        let method = generator.generate_member_function_definition();
        generator.current_class.pop();

        assert!(constructor.starts_with("constructor (self:Widget*"));
        assert!(method.contains("(self:Widget*"));
    }

    #[test]
    fn generated_program_always_includes_main() {
        let grammar = minimal_grammar();
        let mut rng = StdRng::seed_from_u64(31);
        let mut generator = Generator::new(&grammar, &mut rng, 2);

        let ast = generator.generate_ast().unwrap();
        let rendered = generator.stringify_op(&ast.definition);

        assert!(rendered.contains("fn main() i32 {}"));
    }

    #[test]
    fn generated_program_includes_non_main_content() {
        let grammar = minimal_grammar();
        let mut rng = StdRng::seed_from_u64(37);
        let mut generator = Generator::new(&grammar, &mut rng, 0);

        let ast = generator.generate_ast().unwrap();
        let rendered = generator.stringify_op(&ast.definition);

        assert!(rendered.contains("fn main() i32 {}"));
        assert_ne!(rendered, "fn main() i32 {}");
    }
}
