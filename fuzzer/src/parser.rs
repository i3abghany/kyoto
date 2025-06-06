use crate::ast::Grammar;
use std::io;

pub struct Parser;

impl Parser {
    pub fn new() -> Self {
        Parser {}
    }

    pub fn parse(&self, _grammar_str: &String) -> Result<Grammar, io::Error> {
        todo!("Implement parsing logic here")
    }
}
