use std::vec;

use fuzzer::ast::Grammar;

fn main() {
    let g: Grammar = Grammar::new("../grammar/KyotoLexer.g4", "../grammar/KyotoParser.g4")
        .expect("Failed to create grammar");

    println!("Grammar created successfully with {} rules", g.rules.len());
    for (name, rule) in g.rules.iter() {
        println!("Rule: {}", name);
        println!("  Type: {:?}", rule.rule_type);
        println!("  Definition: {:?}", rule.definition);
        println!("  Skip: {}", rule.skip);
    }
}
