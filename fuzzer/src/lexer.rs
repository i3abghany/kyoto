use rstest::*;

#[derive(Debug, PartialEq, Clone, Copy)]
pub enum TokenType {
    Identifier,
    Fragment,
    Skip,
    Colon,
    Plus,
    Star,
    Dot,
    Or,
    Tilde,
    OpenParen,
    CloseParen,
    Set,
    SemiColon,
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
        Lexer { src, pos: 0 }
    }

    pub fn lex(&mut self) -> Vec<Token> {
        let mut tokens = Vec::new();
        while let Some(token) = self.next_token() {
            tokens.push(token);
        }
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
                        self.pos += 1;
                    } else {
                        break;
                    }
                }
                self.pos += 1;
                TokenType::Identifier
            }
            ':' => {
                self.pos += 1;
                TokenType::Colon
            }
            '+' => {
                self.pos += 1;
                TokenType::Plus
            }
            '*' => {
                self.pos += 1;
                TokenType::Star
            }
            '|' => {
                self.pos += 1;
                TokenType::Or
            }
            '~' => {
                self.pos += 1;
                TokenType::Tilde
            }
            '(' => {
                self.pos += 1;
                TokenType::OpenParen
            }
            ')' => {
                self.pos += 1;
                TokenType::CloseParen
            }
            '[' => {
                self.pos += 1;
                while let Some(next_char) = self.curr() {
                    if next_char == ']' {
                        break;
                    } else if next_char == '\\' {
                        self.pos += 2; // Skip escaped character
                    } else {
                        self.pos += 1;
                    }
                }
                if self.curr() == Some(']') {
                    self.pos += 1;
                }
                TokenType::Set
            }
            '.' => {
                self.pos += 1;
                TokenType::Dot
            }
            ';' => {
                self.pos += 1;
                TokenType::SemiColon
            }
            '\'' => {
                self.pos += 1;
                while let Some(next_char) = self.curr() {
                    if next_char == '\\' {
                        self.pos += 2;
                    } else if next_char == '\'' {
                        break;
                    } else {
                        self.pos += 1;
                    }
                }
                if self.curr() == Some('\'') {
                    self.pos += 1;
                }
                TokenType::String
            }
            '-' if self.peek(1) == Some('>') => {
                self.pos += 2;
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
            token_type: if val == "fragment" {
                TokenType::Fragment
            } else if val == "skip" {
                TokenType::Skip
            } else {
                token_type
            },
            value: val,
        })
    }

    fn skip_whitespace(&mut self) {
        while self.pos < self.src.len() && self.src[self.pos..].starts_with([' ', '\n', '\r', '\t'])
        {
            self.pos += 1;
        }
    }
}

#[rstest]
#[case("X", vec![
    Token { token_type: TokenType::Identifier, value: "X".to_string() }])]
#[case("X;", vec![
    Token { token_type: TokenType::Identifier, value: "X".to_string() }, 
    Token { token_type: TokenType::SemiColon, value: ";".to_string() }])]
#[case("X: Y;", vec![
    Token { token_type: TokenType::Identifier, value: "X".to_string() },
    Token { token_type: TokenType::Colon, value: ":".to_string() },
    Token { token_type: TokenType::Identifier, value: "Y".to_string() },
    Token { token_type: TokenType::SemiColon, value: ";".to_string() }])]
#[case("IDENT: 'XYZ';", vec![
    Token { token_type: TokenType::Identifier, value: "IDENT".to_string() },
    Token { token_type: TokenType::Colon, value: ":".to_string() },
    Token { token_type: TokenType::String, value: "XYZ".to_string() },
    Token { token_type: TokenType::SemiColon, value: ";".to_string() }])]
#[case("STRING_LITERAL: '\"' (~[\"\\\\\r\n] | '\\\\' .)* '\"';", vec![
    Token { token_type: TokenType::Identifier, value: "STRING_LITERAL".to_string() },
    Token { token_type: TokenType::Colon, value: ":".to_string() },
    Token { token_type: TokenType::String, value: "\"".to_string() },
    Token { token_type: TokenType::OpenParen, value: "(".to_string() },
    Token { token_type: TokenType::Tilde, value: "~".to_string() },
    Token { token_type: TokenType::Set, value: "[\"\\\\\r\n]".to_string() },
    Token { token_type: TokenType::Or, value: "|".to_string() },
    Token { token_type: TokenType::String, value: "\\\\".to_string() },
    Token { token_type: TokenType::Dot, value: ".".to_string() },
    Token { token_type: TokenType::CloseParen, value: ")".to_string() },
    Token { token_type: TokenType::Star, value: "*".to_string() },
    Token { token_type: TokenType::String, value: "\"".to_string() },
    Token { token_type: TokenType::SemiColon, value: ";".to_string() }])]
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
    Token { token_type: TokenType::SemiColon, value: ";".to_string() }])]
fn test_lexer(#[case] input: &str, #[case] expected: Vec<Token>) {
    let mut lexer = Lexer::new(input.to_string());
    let tokens = lexer.lex();
    assert_eq!(tokens, expected);
}
