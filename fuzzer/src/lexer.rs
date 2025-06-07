use rstest::*;

#[derive(Debug, PartialEq, Clone, Copy)]
pub enum TokenType {
    Identifier,
    Fragment,
    Skip,
    Lexer,
    Parser,
    Grammar,
    /// EOF keyword
    EOF,
    /// Implicit EOF
    EndOfFile,
    Options,
    Colon,
    Plus,
    Star,
    Dot,
    QuestionMark,
    Or,
    Tilde,
    OpenParen,
    CloseParen,
    OpenCurly,
    CloseCurly,
    OpenAngle,
    CloseAngle,
    Set,
    SemiColon,
    Equal,
    String,
    Arrow,
}

#[derive(Debug, PartialEq, Clone)]
pub struct Token {
    pub token_type: TokenType,
    pub value: String,
}

pub struct Lexer {
    src: String,
    pos: usize,
}

impl Lexer {
    pub fn new(src: String) -> Self {
        Lexer { src: src, pos: 0 }
    }

    pub fn lex(&mut self) -> Vec<Token> {
        let mut tokens = Vec::new();
        while let Some(token) = self.next_token() {
            tokens.push(token);
        }
        tokens.push(Token {
            token_type: TokenType::EndOfFile,
            value: String::new(),
        });
        tokens
    }

    fn peek(&self, offset: usize) -> Option<char> {
        if self.pos + offset < self.src.len() {
            Some(self.src.chars().nth(self.pos + offset).unwrap())
        } else {
            None
        }
    }

    fn curr(&self) -> Option<char> {
        self.peek(0)
    }

    fn next_token(&mut self) -> Option<Token> {
        if self.pos >= self.src.len() {
            return None;
        }

        self.skip_whitespace();

        let start_pos = self.pos;

        let curr_char = self.curr()?;
        let token_type = match curr_char {
            'a'..='z' | 'A'..='Z' | '_' => {
                while let Some(next_char) = self.peek(1) {
                    if next_char.is_alphanumeric() || next_char == '_' {
                        self.advance();
                    } else {
                        break;
                    }
                }
                self.advance();
                TokenType::Identifier
            }
            '#' => {
                while let Some(next_char) = self.curr() {
                    if next_char == '\n' || next_char == ';' {
                        break;
                    }
                    self.advance();
                }
                return self.next_token();
            }
            '/' if self.peek(1) == Some('/') => {
                self.advance_n(2);
                while let Some(next_char) = self.curr() {
                    if next_char == '\n' {
                        break;
                    }
                    self.advance();
                }
                return self.next_token();
            }
            '/' if self.peek(1) == Some('*') => {
                self.advance_n(2);
                while let Some(next_char) = self.curr() {
                    if next_char == '*' && self.peek(1) == Some('/') {
                        self.advance_n(2);
                        break;
                    } else {
                        self.advance();
                    }
                }
                return self.next_token();
            }
            ':' => {
                self.advance();
                TokenType::Colon
            }
            '+' => {
                self.advance();
                TokenType::Plus
            }
            '*' => {
                self.advance();
                TokenType::Star
            }
            '|' => {
                self.advance();
                TokenType::Or
            }
            '~' => {
                self.advance();
                TokenType::Tilde
            }
            '(' => {
                self.advance();
                TokenType::OpenParen
            }
            ')' => {
                self.advance();
                TokenType::CloseParen
            }
            '[' => {
                self.advance();
                while let Some(next_char) = self.curr() {
                    if next_char == ']' {
                        break;
                    } else if next_char == '\\' {
                        self.advance_n(2);
                    } else {
                        self.advance();
                    }
                }
                if self.curr() == Some(']') {
                    self.advance();
                }
                TokenType::Set
            }
            '{' => {
                self.advance();
                TokenType::OpenCurly
            }
            '}' => {
                self.advance();
                TokenType::CloseCurly
            }
            '=' => {
                self.advance();
                TokenType::Equal
            }
            '<' => {
                self.advance();
                TokenType::OpenAngle
            }
            '>' => {
                self.advance();
                TokenType::CloseAngle
            }
            '.' => {
                self.advance();
                TokenType::Dot
            }
            ';' => {
                self.advance();
                TokenType::SemiColon
            }
            '?' => {
                self.advance();
                TokenType::QuestionMark
            }
            '\'' => {
                self.advance();
                while let Some(next_char) = self.curr() {
                    if next_char == '\\' {
                        self.advance_n(2);
                    } else if next_char == '\'' {
                        break;
                    } else {
                        self.advance();
                    }
                }
                if self.curr() == Some('\'') {
                    self.advance();
                }
                TokenType::String
            }
            '-' if self.peek(1) == Some('>') => {
                self.advance_n(2);
                TokenType::Arrow
            }
            _ => return None,
        };

        let (start, end) = if self.src.chars().nth(start_pos) == Some('\'') {
            (start_pos + 1, self.pos - 1)
        } else {
            (start_pos, self.pos)
        };

        let val = self.src[start..end].to_string();
        Some(Token {
            token_type: self.token_from_val_or_else(&val, token_type),
            value: val,
        })
    }

    fn advance_n(&mut self, n: usize) {
        self.pos = (self.pos + n).min(self.src.len());
    }

    fn advance(&mut self) {
        self.advance_n(1);
    }

    fn token_from_val_or_else(&self, val: &str, default: TokenType) -> TokenType {
        match val {
            "fragment" => TokenType::Fragment,
            "skip" => TokenType::Skip,
            "lexer" => TokenType::Lexer,
            "parser" => TokenType::Parser,
            "grammar" => TokenType::Grammar,
            "EOF" => TokenType::EOF,
            "options" => TokenType::Options,
            _ => default,
        }
    }

    fn skip_whitespace(&mut self) {
        while self.curr().map_or(false, |c| c.is_whitespace()) {
            self.advance();
        }
    }
}

macro_rules! eof_token {
    () => {
        Token {
            token_type: TokenType::EndOfFile,
            value: String::new(),
        }
    };
}

#[rstest]
#[case("X", vec![
    Token { token_type: TokenType::Identifier, value: "X".to_string() }, eof_token!()])]
#[case("X;", vec![
    Token { token_type: TokenType::Identifier, value: "X".to_string() }, 
    Token { token_type: TokenType::SemiColon, value: ";".to_string() }, eof_token!()])]
#[case("lexer grammar kyoto;", vec![
    Token { token_type: TokenType::Lexer, value: "lexer".to_string() },
    Token { token_type: TokenType::Grammar, value: "grammar".to_string() },
    Token { token_type: TokenType::Identifier, value: "kyoto".to_string() },
    Token { token_type: TokenType::SemiColon, value: ";".to_string() }, eof_token!()])]
#[case("X: Y;", vec![
    Token { token_type: TokenType::Identifier, value: "X".to_string() },
    Token { token_type: TokenType::Colon, value: ":".to_string() },
    Token { token_type: TokenType::Identifier, value: "Y".to_string() },
    Token { token_type: TokenType::SemiColon, value: ";".to_string() }, eof_token!()])]
#[case("IDENT: 'XYZ';", vec![
    Token { token_type: TokenType::Identifier, value: "IDENT".to_string() },
    Token { token_type: TokenType::Colon, value: ":".to_string() },
    Token { token_type: TokenType::String, value: "XYZ".to_string() },
    Token { token_type: TokenType::SemiColon, value: ";".to_string() }, eof_token!()])]
#[case("STRING_LITERAL: '\"' (~[\"\\\\\\r\\n] | '\\\\' .)* '\"';", vec![
    Token { token_type: TokenType::Identifier, value: "STRING_LITERAL".to_string() },
    Token { token_type: TokenType::Colon, value: ":".to_string() },
    Token { token_type: TokenType::String, value: "\"".to_string() },
    Token { token_type: TokenType::OpenParen, value: "(".to_string() },
    Token { token_type: TokenType::Tilde, value: "~".to_string() },
    Token { token_type: TokenType::Set, value: "[\"\\\\\\r\\n]".to_string() },
    Token { token_type: TokenType::Or, value: "|".to_string() },
    Token { token_type: TokenType::String, value: "\\\\".to_string() },
    Token { token_type: TokenType::Dot, value: ".".to_string() },
    Token { token_type: TokenType::CloseParen, value: ")".to_string() },
    Token { token_type: TokenType::Star, value: "*".to_string() },
    Token { token_type: TokenType::String, value: "\"".to_string() },
    Token { token_type: TokenType::SemiColon, value: ";".to_string() }, eof_token!()])]
#[case("CHAR_LITERAL: '\\'' (~['\\\\\\r\\n] | '\\\\' .) '\\'';", vec![
    Token { token_type: TokenType::Identifier, value: "CHAR_LITERAL".to_string() },
    Token { token_type: TokenType::Colon, value: ":".to_string() },
    Token { token_type: TokenType::String, value: "\\'".to_string() },
    Token { token_type: TokenType::OpenParen, value: "(".to_string() },
    Token { token_type: TokenType::Tilde, value: "~".to_string() },
    Token { token_type: TokenType::Set, value: "['\\\\\\r\\n]".to_string() },
    Token { token_type: TokenType::Or, value: "|".to_string() },
    Token { token_type: TokenType::String, value: "\\\\".to_string() },
    Token { token_type: TokenType::Dot, value: ".".to_string() },
    Token { token_type: TokenType::CloseParen, value: ")".to_string() },
    Token { token_type: TokenType::String, value: "\\'".to_string() },
    Token { token_type: TokenType::SemiColon, value: ";".to_string() }, eof_token!()])]
#[case("ruleName: X EOF;", vec![
    Token { token_type: TokenType::Identifier, value: "ruleName".to_string() },
    Token { token_type: TokenType::Colon, value: ":".to_string() },
    Token { token_type: TokenType::Identifier, value: "X".to_string() },
    Token { token_type: TokenType::EOF, value: "EOF".to_string() },
    Token { token_type: TokenType::SemiColon, value: ";".to_string() }, eof_token!()])]
#[case("ruleName: X+;", vec![
    Token { token_type: TokenType::Identifier, value: "ruleName".to_string() },
    Token { token_type: TokenType::Colon, value: ":".to_string() },
    Token { token_type: TokenType::Identifier, value: "X".to_string() },
    Token { token_type: TokenType::Plus, value: "+".to_string() },
    Token { token_type: TokenType::SemiColon, value: ";".to_string() }, eof_token!()])]
#[case("NAME: [a-z] -> skip;", vec![
    Token { token_type: TokenType::Identifier, value: "NAME".to_string() },
    Token { token_type: TokenType::Colon, value: ":".to_string() },
    Token { token_type: TokenType::Set, value: "[a-z]".to_string() },
    Token { token_type: TokenType::Arrow, value: "->".to_string() },
    Token { token_type: TokenType::Skip, value: "skip".to_string() },
    Token { token_type: TokenType::SemiColon, value: ";".to_string() }, eof_token!()])]
#[case("X: Y | /* empty */;", vec![
    Token { token_type: TokenType::Identifier, value: "X".to_string() },
    Token { token_type: TokenType::Colon, value: ":".to_string() },
    Token { token_type: TokenType::Identifier, value: "Y".to_string() },
    Token { token_type: TokenType::Or, value: "|".to_string() },
    Token { token_type: TokenType::SemiColon, value: ";".to_string() }, eof_token!()])]
fn test_lexer(#[case] input: &str, #[case] expected: Vec<Token>) {
    let mut lexer = Lexer::new(input.to_string());
    let tokens = lexer.lex();
    assert_eq!(tokens, expected);
}
