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
        let content = Self::strip_content(&content);
        let parser = Parser::new();
        match parser.parse(&content) {
            Ok(grammar) => grammar,
            Err(e) => panic!("Failed to parse grammar: {}", e),
        }
    }

    fn strip_content(content: &str) -> String {
        let rp = "/\\*[^*]*\\*+(?:[^/*][^*]*\\*+)*/";
        let re = regex::Regex::new(rp).unwrap();
        let content = content
            .lines()
            .map(|line| re.replace_all(&line, "").to_string());

        let rp = "(#|//).*";
        let re = regex::Regex::new(rp).unwrap();
        let content = content.map(|line| re.replace_all(&line, "").to_string());

        content
            .map(|line| line.trim_end().to_string())
            .collect::<Vec<String>>()
            .join("\n")
    }
}

mod test {
    use rstest::rstest;

    #[rstest]
    #[case("test;", "test;")]
    #[case("test; # comment", "test;")]
    #[case("test; /* comment */", "test;")]
    #[case("test; /* comment */ # comment", "test;")]
    #[case("test;/* nested # comment */test2;", "test;test2;")]
    #[case("slash comment // comment", "slash comment")]
    #[case("empty slash comment //", "empty slash comment")]
    fn test_strip_content(#[case] input: &str, #[case] expected: &str) {
        let result = crate::ast::Grammar::strip_content(input);
        assert_eq!(result, expected);
    }
}
