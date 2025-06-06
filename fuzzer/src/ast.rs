use std::io::Read;

use crate::parser::Parser;

pub struct Grammar {
    pub rules: Vec<Rule>,
}

#[derive(Debug, Clone, Copy)]
pub enum RuleType {
    Lexer,
    Parser,
    Fragment,
}

pub enum Op {
    Terminal(String),
    RuleRef(String),

    ZeroOrMore(Box<Op>),
    OneOrMore(Box<Op>),
    Optional(Box<Op>),

    Sequence(Vec<Op>),
    Alternative(Vec<Op>),

    CharRange(char, char),
    Set(Vec<Op>),

    Eof,
    AnyChar,
}

pub struct Rule {
    pub name: String,
    pub rule_type: RuleType,
    pub op: Op,
    pub is_fragment: bool,
}

impl Grammar {
    pub fn new(files: Vec<String>) -> Self {
        let mut content = String::new();
        for file in files {
            let mut f = std::fs::File::open(&file).expect("Failed to open file");
            if let Err(e) = f.read_to_string(&mut content) {
                eprintln!("Error reading file {}: {}", file, e);
            }
        }
        let parser = Parser::new();
        match parser.parse(&content) {
            Ok(grammar) => grammar,
            Err(e) => panic!("Failed to parse grammar: {}", e),
        }
    }
}
