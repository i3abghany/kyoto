use crate::ast::{Grammar, Op, Rule};
use rand::Rng;
use std::io;

pub struct Generator<'a, R: Rng> {
    grammar: &'a Grammar,
    rng: &'a mut R,
    max_depth: usize,
}

impl<'a, R: Rng> Generator<'a, R> {
    pub fn new(grammar: &'a Grammar, rng: &'a mut R, max_depth: usize) -> Self {
        Generator {
            grammar,
            rng,
            max_depth,
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

        let op = self.generate_op(&entry_point.definition, 0)?;
        Ok(Rule {
            name: entry_point.name,
            rule_type: entry_point.rule_type,
            definition: op,
            skip: entry_point.skip,
        })
    }

    pub fn generate_op(&mut self, op: &Op, depth: usize) -> io::Result<Op> {
        if depth >= self.max_depth && !op.is_trivial_rule() {
            return Ok(Op::Empty);
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
            Op::Optional(op) => {
                let inner_op = self.generate_op(op, depth)?;
                match inner_op {
                    Op::Empty => Ok(Op::Empty),
                    _ => Ok(Op::Optional(Box::new(inner_op))),
                }
            }
            Op::Not(op) => {
                let inner_op = self.generate_op(op, depth)?;
                match inner_op {
                    Op::Empty => Ok(Op::Empty),
                    _ => Ok(Op::Not(Box::new(inner_op))),
                }
            }
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
                    result.push_str(&s);
                    if [
                        "cdecl",
                        "fn",
                        "class",
                        "var",
                        "true",
                        "false",
                        "return",
                        "if",
                        "else",
                        "for",
                        "match",
                        "default",
                        "constructor",
                        "new",
                        "free",
                    ]
                    .contains(&s.as_str())
                    {
                        result.push(' ');
                    }
                }
                result
            }
            Op::ZeroOrMore(op) => self.stringify_op(op),
            Op::OneOrMore(op) => self.stringify_op(op),
            Op::Parentheses(op) => self.stringify_op(op),
            Op::Optional(op) => {
                let inner_op = self.stringify_op(op);
                if self.rng.random_range(0..2) == 0 {
                    inner_op
                } else {
                    String::new()
                }
            }
            _ => String::new(),
        }
    }

    fn generate_set(&mut self, ops: &[Op], depth: usize) -> io::Result<Op> {
        let mut s = String::new();
        for op in ops {
            match self.generate_op(op, depth + 1)? {
                Op::Terminal(c) => s.push_str(&c),
                Op::CharRange(start, end) => {
                    let generated_char = self.generate_char_in_range(start, end);
                    s.push(generated_char);
                }
                Op::Empty => {}
                _ => {
                    return Err(io::Error::new(
                        io::ErrorKind::InvalidData,
                        "Unsupported operation in set",
                    ));
                }
            }
        }
        Ok(Op::Terminal(s))
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
                if name == "IDENTIFIER" {
                    return Ok(Op::Terminal(self.generate_identifier()));
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
        let mut identifier = String::new();
        let len = self.rng.random_range(1..30);
        identifier.reserve(len);
        for _ in 0..len {
            identifier.push(self.generate_char_in_range('\u{0080}', '\u{FFFF}'));
        }
        identifier
    }

    fn generate_expression(&mut self, op: &Op, depth: usize) -> io::Result<Op> {
        let prune = self.rng.random_ratio(1, 10);

        if !prune {
            return self.generate_op(op, depth + 1);
        }

        let simple_expressions = vec![
            Op::Terminal("1".to_string()),
            Op::Terminal("1321".to_string()),
            Op::Terminal("0".to_string()),
            Op::Terminal("192830".to_string()),
            Op::Terminal("true".to_string()),
            Op::Terminal("false".to_string()),
            Op::RuleRef("CHAR_LITERAL".to_string()),
            Op::RuleRef("STRING_LITERAL".to_string()),
            Op::RuleRef("IDENTIFIER".to_string()),
        ];

        let chosen_op = self.rng.random_range(0..simple_expressions.len());
        return self.generate_op(&simple_expressions[chosen_op], depth + 1);
    }
}
