# Anonymous Functions in Kyoto

This document describes the design of anonymous functions in Kyoto. It is a work in progress and may change as the implementation evolves.

## Overview

Anonymous functions are functions without a name. They can be used as arguments to high-order functions, or as a way to create a function on the fly without having to define it separately. In Kyoto, we support anonymous functions using a syntax similar to lambda expressions in other languages.

Right now, anonymous functions are isolated from the environment calling them. They cannot capture variables from the surrounding scope, and they cannot access any state outside of their parameters.

## Syntax

The syntax for anonymous functions in Kyoto is as follows:

```
fn (arg1: Type1, arg2: Type2, ...) ReturnType {
    // function body
}
```

For example, to create an anonymous function that adds two numbers, you can write:

```
fn (x: i32, y: i32) i32 {
    x + y
}
```

The anonymous functions have the type `fn (Type1, Type2, ...) ReturnType`, which can be used to specify the type of the function when passing it as an argument to another function.

## Usage

Anonymous functions can be used in various contexts, such as:

```
fn consume(f: fn (i32, i32) i32) i32 {
    var result: i32 = f(2, 3);
    return result;
}

fn main() i32 {
    var add: fn (i32, i32) i32 = fn (x: i32, y: i32) i32 {
        return x + y;
    };
    var result: i32 = consume(add);
    return result;
}

```

Another example of using an anonymous function directly as an argument:

```
fn main() i32 {
    var result: i32 = consume(fn (x: i32, y: i32) i32 {
        return x * y;
    });
    return result;
}
```

Functions can be called directly as well:

```
fn main() i32 {
    var result: i32 = (fn (x: i32, y: i32) i32 {
        var res: i32 = x - y;
        return res;
    })(5, 2);
    return result;
}
```
    