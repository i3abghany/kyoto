use rstest::*;

#[derive(Debug, PartialEq, Clone, Copy)]
pub enum TokenType {
    Identifier,
    Fragment,
    Skip,
    Lexer,
    Grammar,
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
        Lexer {
            src: Lexer::strip_content(&src),
            pos: 0,
        }
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
                        self.advance();
                    } else {
                        break;
                    }
                }
                self.advance();
                TokenType::Identifier
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
            '.' => {
                self.advance();
                TokenType::Dot
            }
            ';' => {
                self.advance();
                TokenType::SemiColon
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
            "grammar" => TokenType::Grammar,
            _ => default,
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

    fn skip_whitespace(&mut self) {
        while self.pos < self.src.len() && self.curr().map_or(false, |c| c.is_whitespace()) {
            self.advance();
        }
    }
}

#[rstest]
#[case("X", vec![
    Token { token_type: TokenType::Identifier, value: "X".to_string() }])]
#[case("X;", vec![
    Token { token_type: TokenType::Identifier, value: "X".to_string() }, 
    Token { token_type: TokenType::SemiColon, value: ";".to_string() }])]
#[case("lexer grammar kyoto;", vec![
    Token { token_type: TokenType::Lexer, value: "lexer".to_string() },
    Token { token_type: TokenType::Grammar, value: "grammar".to_string() },
    Token { token_type: TokenType::Identifier, value: "kyoto".to_string() },
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

#[rstest]
#[case("test;", "test;")]
#[case("test; # comment", "test;")]
#[case("test; /* comment */", "test;")]
#[case("test; /* comment */ # comment", "test;")]
#[case("test;/* nested # comment */test2;", "test;test2;")]
#[case("slash comment // comment", "slash comment")]
#[case("empty slash comment //", "empty slash comment")]
fn test_strip_content(#[case] input: &str, #[case] expected: &str) {
    let result = Lexer::strip_content(input);
    assert_eq!(result, expected);
}
