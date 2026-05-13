# Function Overloading in Kyoto

This document describes function and constructor overloading in Kyoto. The design is intentionally small and strict so overload resolution stays predictable.

## Overview

Functions and constructors can share the same name when their parameter lists are distinguishable. Kyoto considers both the number of parameters and their types when selecting an overload.

For example, these two functions are distinct overloads:

```
fn pick(x: i32) i32 {
    return 32;
}

fn pick(x: i64) i32 {
    return 64;
}
```

Calling `pick` selects the overload whose parameter type best matches the argument type:

```
fn main() i32 {
    var wide: i64 = 7;
    return pick(1) + pick(wide); // Calls pick(i32) + pick(i64)
}
```

## Constructors

Constructors follow the same overload rules. The implicit destination object is part of the compiled constructor signature, but callers only provide the user-facing constructor arguments.

```
class Point {
    var x: i32;

    constructor(self: Point*, x: i32) {
        self.x = 32;
    }

    constructor(self: Point*, x: i64) {
        self.x = 64;
    }
}

fn main() i32 {
    var wide: i64 = 7;
    var p1: Point = Point(1);
    var p2: Point = Point(wide);
    return p1.x + p2.x;
}
```

## Resolution Rules

Kyoto resolves overloads in this order:

1. Exact parameter type matches are preferred.
2. If there is no exact match, Kyoto may use a single compatible primitive conversion.
3. If more than one overload is compatible by conversion, the call is ambiguous.
4. Return type is not used to distinguish overloads.

The primitive conversion rule is deliberately conservative. Kyoto should not grow overload ranking into a complex system of rules.

For example, this call is ambiguous because the literal can fit both overloads:

```
fn pick(x: i8) i32 {
    return 8;
}

fn pick(x: char) i32 {
    return 1;
}

fn main() i32 {
    return pick(1); // error: ambiguous
}
```

Use an explicit type or cast when you want to choose a specific overload.

## Note on Variadic Functions

Kyoto does not support Kyoto-defined variadic functions. `fn foo(...) T` is an error. However, Kyoto can call C variadic functions declared with `cdecl`.