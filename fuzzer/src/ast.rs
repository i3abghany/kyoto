use std::{collections::HashMap, io::Read};

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

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Rule {
    pub name: String,
    pub rule_type: RuleType,
    pub definition: Op,
    pub skip: bool,
}

impl Grammar {
    pub fn new(lexer_file: &str, parser_file: &str) -> Result<Grammar, std::io::Error> {
        Ok(Grammar {
            rules: HashMap::new(),
            entry_points: vec![],
        })
    }
}
