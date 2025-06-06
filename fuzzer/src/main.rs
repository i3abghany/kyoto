use fuzzer::lexer::Lexer;

fn main() {
    let src = std::fs::read_to_string("inp.txt").expect("Failed to read file");

    println!("Source:\n{}\n", src);
    let mut lexer = Lexer::new(src.to_string());
    let tkns = lexer.lex();
    println!("{:#?}", tkns);
}
