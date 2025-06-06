use std::vec;

use fuzzer::lexer::Lexer;

fn main() {
    let files = vec![
        "../grammar/KyotoLexer.g4".to_string(),
        "../grammar/KyotoParser.g4".to_string(),
    ];

    let contents = files
        .iter()
        .map(|file| std::fs::read_to_string(file).expect("Failed to read file"))
        .collect::<Vec<_>>()
        .join("\n");

    let tkns = Lexer::new(contents).lex();
    print!("{:#?}", tkns);
}
