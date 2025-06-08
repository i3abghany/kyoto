use std::collections::HashMap;

use crate::parser::Parser;

pub struct Grammar {
    pub rules: HashMap<String, Rule>,
    pub entry_points: Vec<String>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RuleType {
    Fragment,
    Other,
}

#[derive(Debug, Clone, PartialEq, Eq, Ord, PartialOrd)]
pub enum Op {
    Terminal(String),
    RuleRef(String),

    ZeroOrMore(Box<Op>),
    OneOrMore(Box<Op>),
    Optional(Box<Op>),
    Not(Box<Op>),
    Parentheses(Box<Op>),

    Sequence(Vec<Op>),
    Alternative(Vec<Op>),

    CharRange(char, char),
    Set(Vec<Op>),

    AnyChar,
    Empty,
    Skip,

    EofDefinition,
}

impl Op {
    pub fn is_trivial_rule(&self) -> bool {
        matches!(self, Op::Set(_) | Op::CharRange(_, _))
            || matches!(self, Op::Terminal(_) | Op::AnyChar)
            || matches!(self, Op::RuleRef(name) if name.chars().all(|c| c.is_uppercase() || c == '_'))
            || matches!(self, Op::RuleRef(name) if ["CHAR", "VOID", "STRING", "I8", "I16", "I32", "I64", "U8", "U16", "U32", "U64", "F32", "F64", "type"].contains(&name.as_str()))
            || matches!(self, Op::RuleRef(name) if name == "IDENTIFIER")
            || matches!(self, Op::Sequence(ops) if ops.iter().all(|op| op.is_trivial_rule()))
            || matches!(self, Op::ZeroOrMore(op) if op.is_trivial_rule())
            || matches!(self, Op::OneOrMore(op) if op.is_trivial_rule())
            || matches!(self, Op::Optional(op) if op.is_trivial_rule())
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Rule {
    pub name: String,
    pub rule_type: RuleType,
    pub definition: Op,
    pub skip: bool,
}

impl Grammar {
    pub fn new(lexer_file: &str, parser_file: &str) -> Result<Grammar, std::io::Error> {
        let lexer_src = std::fs::read_to_string(lexer_file)?;
        let parser_src = std::fs::read_to_string(parser_file)?;

        let lexer_rules = Parser::new()
            .parse_lexer_rules(lexer_src)
            .expect("Failed to parse lexer rules");

        let parser_rules = Parser::new()
            .parse_parser_rules(parser_src)
            .expect("Failed to parse parser rules");

        let mut rules = HashMap::new();
        rules.extend(lexer_rules.rules);
        rules.extend(parser_rules.rules);

        let entry_points = vec!["program".to_string()];
        Ok(Grammar {
            rules,
            entry_points,
        })
    }
}
