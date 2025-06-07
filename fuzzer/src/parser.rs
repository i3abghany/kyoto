use crate::ast;
use crate::ast::Grammar;
use crate::lexer::{Token, TokenType};
use rstest::rstest;
use std::vec::IntoIter;
use std::{io, iter::Peekable, str::Chars};

pub struct SetParser;

impl SetParser {
    pub fn new() -> Self {
        SetParser
    }

    pub fn parse_set(&self, set_str: &str) -> Result<ast::Op, io::Error> {
        let mut chars = set_str.chars().peekable();

        self.expect_char(&mut chars, '[', "Set must start with '['")?;

        let mut ranges: Vec<ast::Op> = Vec::new();
        let mut pending_char_start: Option<char> = None;

        while let Some(c) = chars.peek().copied() {
            match c {
                ']' => {
                    chars.next();
                    break;
                }
                '-' => {
                    chars.next();
                    let start_char = pending_char_start.take().ok_or_else(|| {
                        Self::error("Invalid set syntax: '-' without a preceding character.")
                    })?;
                    let end_char = self
                        .parse_char_in_set(&mut chars, "Expected character after '-' in range")?;
                    ranges.push(ast::Op::CharRange(start_char, end_char));
                }
                '\\' => {
                    chars.next();
                    let escaped_char = self.parse_escaped_char(&mut chars)?;
                    if let Some(start_char) = pending_char_start.take() {
                        ranges.push(ast::Op::CharRange(start_char, start_char));
                    }
                    pending_char_start = Some(escaped_char);
                }
                _ => {
                    chars.next();
                    if let Some(start_char) = pending_char_start.take() {
                        ranges.push(ast::Op::CharRange(start_char, start_char));
                    }
                    pending_char_start = Some(c);
                }
            }
        }

        if let Some(c) = pending_char_start {
            ranges.push(ast::Op::CharRange(c, c));
        }

        if chars.next().is_some() {
            return Err(Self::error(
                "Unexpected characters after closing ']' in set.",
            ));
        }

        if ranges.is_empty() {
            Ok(ast::Op::Set(vec![]))
        } else {
            Ok(ast::Op::Set(ranges))
        }
    }

    fn parse_char_in_set(
        &self,
        chars: &mut Peekable<Chars>,
        error_msg: &str,
    ) -> Result<char, io::Error> {
        if let Some(c) = chars.peek().copied() {
            if c == '\\' {
                chars.next();
                self.parse_escaped_char(chars)
            } else {
                chars.next();
                Ok(c)
            }
        } else {
            Err(Self::error(error_msg))
        }
    }

    fn parse_escaped_char(&self, chars: &mut Peekable<Chars>) -> Result<char, io::Error> {
        match chars.next() {
            Some('n') => Ok('\n'),
            Some('t') => Ok('\t'),
            Some('r') => Ok('\r'),
            Some('u') => self.parse_unicode_escape_sequence(chars),
            Some(other_char) => Ok(other_char),
            None => Err(Self::error("Incomplete escape sequence.")),
        }
    }

    fn parse_unicode_escape_sequence(
        &self,
        chars: &mut Peekable<Chars>,
    ) -> Result<char, io::Error> {
        let mut hex_str = String::with_capacity(4);
        for _ in 0..4 {
            if let Some(hex_char) = chars.next() {
                if hex_char.is_ascii_hexdigit() {
                    hex_str.push(hex_char);
                } else {
                    return Err(Self::error(&format!(
                        "Invalid hex digit in unicode escape sequence: '{}'",
                        hex_char
                    )));
                }
            } else {
                return Err(Self::error(
                    "Incomplete unicode escape sequence: expected 4 hex digits.",
                ));
            }
        }

        u32::from_str_radix(&hex_str, 16)
            .ok()
            .and_then(char::from_u32)
            .ok_or_else(|| Self::error(&format!("Invalid unicode character from hex: {}", hex_str)))
    }

    fn expect_char(
        &self,
        chars: &mut Peekable<Chars>,
        expected: char,
        error_msg: &str,
    ) -> Result<(), io::Error> {
        match chars.next() {
            Some(c) if c == expected => Ok(()),
            _ => Err(Self::error(error_msg)),
        }
    }

    fn error(message: &str) -> io::Error {
        io::Error::new(io::ErrorKind::InvalidData, message)
    }
}

pub struct Parser {
    tokens: Peekable<IntoIter<Token>>,
    current_token: Option<Token>,
    in_skip_rule: bool,
}

impl Parser {
    pub fn new() -> Self {
        Parser {
            tokens: Vec::new().into_iter().peekable(),
            current_token: None,
            in_skip_rule: false,
        }
    }

    fn parse(&mut self) -> Result<Grammar, io::Error> {
        let mut rules = std::collections::HashMap::<String, ast::Rule>::new();
        let entry_points = Vec::new();

        while let Some(token) = self.peek_token()? {
            if token.token_type == TokenType::EndOfFile {
                self.next_token()?;
                break;
            }

            let mut rule = self.parse_rule()?;
            if self.in_skip_rule {
                rule.skip = true;
                self.in_skip_rule = false;
            }
            let rule_name = rule.name.clone();
            rules.insert(rule_name.clone(), rule);
        }

        Ok(Grammar {
            rules,
            entry_points,
        })
    }

    pub fn parse_lexer_rules(&mut self, grammar_str: String) -> Result<Grammar, io::Error> {
        let lexer_tokens = crate::lexer::Lexer::new(grammar_str).lex();
        self.tokens = lexer_tokens.into_iter().peekable();
        self.current_token = self.tokens.next();

        self.expect_and_consume_n(&[
            TokenType::Lexer,
            TokenType::Grammar,
            TokenType::Identifier,
            TokenType::SemiColon,
        ])?;

        self.parse()
    }

    pub fn parse_parser_rules(&mut self, grammar_str: String) -> Result<Grammar, io::Error> {
        let lexer_tokens = crate::lexer::Lexer::new(grammar_str).lex();
        self.tokens = lexer_tokens.into_iter().peekable();
        self.current_token = self.tokens.next();

        self.expect_and_consume_n(&[
            TokenType::Parser,
            TokenType::Grammar,
            TokenType::Identifier,
            TokenType::SemiColon,
        ])?;

        self.expect_and_consume_n(&[
            TokenType::Options,
            TokenType::OpenCurly,
            TokenType::Identifier,
            TokenType::Equal,
            TokenType::Identifier,
            TokenType::SemiColon,
            TokenType::CloseCurly,
        ])?;

        self.parse()
    }

    fn parse_rule_type(&mut self) -> Result<ast::RuleType, io::Error> {
        if let Some(token) = self.peek_token()? {
            match token.token_type {
                TokenType::Fragment => {
                    self.next_token()?;
                    Ok(ast::RuleType::Fragment)
                }
                _ => Ok(ast::RuleType::Other),
            }
        } else {
            Err(io::Error::new(
                io::ErrorKind::InvalidData,
                "Expected rule type or identifier for rule name, found EOF",
            ))
        }
    }

    fn parse_rule(&mut self) -> Result<ast::Rule, io::Error> {
        let rule_type = self.parse_rule_type()?;
        let rule_name_token = self.expect(TokenType::Identifier)?;
        let rule_name = rule_name_token.value;
        self.expect(TokenType::Colon)?;

        let definition = self.parse_rule_definition()?;

        self.expect(TokenType::SemiColon)?;

        Ok(ast::Rule {
            name: rule_name,
            rule_type,
            definition,
            skip: false,
        })
    }

    pub fn parse_rule_definition(&mut self) -> Result<ast::Op, io::Error> {
        let mut alternatives = Vec::new();
        alternatives.push(self.parse_sequence()?);

        while let Some(token) = self.peek_token()? {
            if token.token_type == TokenType::Or {
                self.next_token()?;
                alternatives.push(self.parse_sequence()?);
            } else {
                break;
            }
        }

        if alternatives.len() == 1 {
            Ok(alternatives.remove(0))
        } else {
            Ok(ast::Op::Alternative(alternatives))
        }
    }

    fn parse_sequence(&mut self) -> Result<ast::Op, io::Error> {
        let mut sequence = Vec::new();

        loop {
            if let Some(token) = self.peek_token()? {
                match token.token_type {
                    TokenType::SemiColon
                    | TokenType::EndOfFile
                    | TokenType::Or
                    | TokenType::CloseParen => {
                        break;
                    }
                    _ => {
                        let op = self.parse_operation()?;
                        if op != ast::Op::Skip {
                            sequence.push(op);
                        }
                    }
                }
            } else {
                break;
            }
        }

        if sequence.is_empty() {
            Ok(ast::Op::Empty)
        } else if sequence.len() == 1 {
            Ok(sequence.remove(0))
        } else {
            Ok(ast::Op::Sequence(sequence))
        }
    }

    fn parse_operation(&mut self) -> Result<ast::Op, io::Error> {
        let mut op = self.parse_primary_operation()?;
        if op == ast::Op::Skip {
            return Ok(op);
        }

        if op == ast::Op::Skip {
            return Ok(op);
        }

        while let Some(token) = self.peek_token()? {
            match token.token_type {
                TokenType::Plus => {
                    self.next_token()?;
                    op = ast::Op::OneOrMore(Box::new(op));
                }
                TokenType::Star => {
                    self.next_token()?;
                    op = ast::Op::ZeroOrMore(Box::new(op));
                }
                TokenType::QuestionMark => {
                    self.next_token()?;
                    op = ast::Op::Optional(Box::new(op));
                }
                _ => break,
            }
        }
        Ok(op)
    }

    fn parse_primary_operation(&mut self) -> Result<ast::Op, io::Error> {
        let tok = self.next_token()?.ok_or_else(|| {
            io::Error::new(
                io::ErrorKind::UnexpectedEof,
                "Expected an operation, found EOF",
            )
        })?;
        match tok.token_type {
            TokenType::String => self.parse_terminal_operation(tok),
            TokenType::Identifier => self.parse_rule_reference(tok),
            TokenType::OpenParen => {
                let inner_op = self.parse_rule_definition()?;
                self.expect(TokenType::CloseParen)?;
                Ok(inner_op)
            }
            TokenType::Tilde => {
                let inner_op = self.parse_rule_definition()?;
                Ok(ast::Op::Not(Box::new(inner_op)))
            }
            TokenType::Set => SetParser::new().parse_set(&tok.value),
            TokenType::Dot => Ok(ast::Op::AnyChar),
            TokenType::EOF => Ok(ast::Op::EofDefinition),
            TokenType::Arrow => {
                self.expect(TokenType::Skip)?;
                self.in_skip_rule = true;
                Ok(ast::Op::Skip)
            }
            TokenType::OpenAngle => {
                // skip <assoc = right> rule for now...
                self.expect_and_consume_n(&[
                    TokenType::Identifier,
                    TokenType::Equal,
                    TokenType::Identifier,
                    TokenType::CloseAngle,
                ])?;
                self.parse_primary_operation()
            }
            _ => Err(io::Error::new(
                io::ErrorKind::InvalidData,
                format!("Unexpected token type: {:?}", tok.token_type),
            )),
        }
    }

    fn parse_terminal_operation(&mut self, token: Token) -> Result<ast::Op, io::Error> {
        if token.token_type == TokenType::String {
            return Ok(ast::Op::Terminal(token.value));
        }

        Err(io::Error::new(
            io::ErrorKind::InvalidData,
            format!(
                "Expected a terminal or rule reference, found {:?}",
                token.token_type
            ),
        ))
    }

    fn parse_rule_reference(&mut self, token: Token) -> Result<ast::Op, io::Error> {
        if token.token_type == TokenType::Identifier {
            return Ok(ast::Op::RuleRef(token.value));
        }

        Err(io::Error::new(
            io::ErrorKind::InvalidData,
            format!("Expected a rule reference, found {:?}", token.token_type),
        ))
    }

    fn peek_token(&mut self) -> Result<Option<&Token>, io::Error> {
        if self.current_token.is_some() {
            return Ok(self.current_token.as_ref());
        }

        if self.tokens.peek().is_some() {
            self.current_token = self.tokens.next();
            return Ok(self.current_token.as_ref());
        }

        Ok(None)
    }

    fn next_token(&mut self) -> Result<Option<Token>, io::Error> {
        if let Some(token) = self.current_token.take() {
            self.current_token = self.tokens.next();
            return Ok(Some(token));
        }

        if let Some(token) = self.tokens.next() {
            self.current_token = Some(token);
            return Ok(self.current_token.clone());
        }

        Ok(None)
    }

    fn expect(&mut self, expected: TokenType) -> Result<Token, io::Error> {
        if let Some(token) = self.current_token.as_ref() {
            if token.token_type == expected {
                let token = self.current_token.take().unwrap();
                self.current_token = self.tokens.next();
                return Ok(token);
            } else {
                return Err(io::Error::new(
                    io::ErrorKind::UnexpectedEof,
                    format!("Expected token type: {:?}", expected),
                ));
            }
        }

        Err(io::Error::new(
            io::ErrorKind::UnexpectedEof,
            "No current token available",
        ))
    }

    fn expect_and_consume_n(&mut self, token_types: &[TokenType]) -> Result<(), io::Error> {
        for &expected in token_types {
            self.expect(expected)?;
        }

        Ok(())
    }
}

#[rstest]
#[case("[a-z]", ast::Op::Set(vec![ast::Op::CharRange('a', 'z')]))]
#[case("[0-9]", ast::Op::Set(vec![ast::Op::CharRange('0', '9')]))]
#[case("[a-zA-Z]", ast::Op::Set(vec![ast::Op::CharRange('a', 'z'), ast::Op::CharRange('A', 'Z')]))]
#[case("[ab]", ast::Op::Set(vec![ast::Op::CharRange('a', 'a'), ast::Op::CharRange('b', 'b')]))]
#[case("[a-zA-Z0-9]", ast::Op::Set(vec![ast::Op::CharRange('a', 'z'), ast::Op::CharRange('A', 'Z'), ast::Op::CharRange('0', '9')]))]
#[case("[\\u0041-\\u005A]", ast::Op::Set(vec![ast::Op::CharRange('A', 'Z')]))]
#[case("[\\n\\t]", ast::Op::Set(vec![ast::Op::CharRange('\n', '\n'), ast::Op::CharRange('\t', '\t')]))]
#[case("[A-Za-z_1]", ast::Op::Set(vec![ast::Op::CharRange('A', 'Z'), ast::Op::CharRange('a', 'z'), ast::Op::CharRange('_', '_'), ast::Op::CharRange('1', '1')]))]
fn test_parse_set(#[case] set_str: &str, #[case] expected: ast::Op) {
    let parser = SetParser::new();
    let result = parser.parse_set(set_str).unwrap();
    assert_eq!(result, expected, "Failed to parse set: {}", set_str);
}
