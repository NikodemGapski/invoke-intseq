# Invoke intseq
A C++ programming course project regarding metaprogramming.

## Problem statement
The goal of this project is to implement a certain generalization of `std::invoke`, so that it treats arguments of type `std::integer_sequence` in a special way.

In the header file `invoke_intseq.h` you should implement a template:
```c++
template <class F, class... Args>
template result_type invoke_intseq(F&& f, Args&&... args);
```

1. If none of the arguments is of type `std::integer_sequence`, the call to `invoke_intseq(f, args...)` should have the same effect as a call to `std::invoke(f, args...)`.

2. If there is at least one such argument, the function `f` should be invoked for all possible combinations of elements encoded in the `std::integer_sequence`'s. Formally, let `args...` be a sequence of arguments `a_1, a_2, ..., a_n` and let `a_i` be the first argument of type `std::integer_sequence<T, j_1, j_2, ..., j_m>`. Then `invoke_intseq(f, a_1, ..., a_n)` is defined as a series of recursive calls:
```c++
invoke_intseq(f, a_1, ..., a_{i - 1}, std::integral_constant<T, j_1>, a_{i + 1}, ..., a_n),
invoke_intseq(f, a_1, ..., a_{i - 1}, std::integral_constant<T, j_2>, a_{i + 1}, ..., a_n),
...
invoke_intseq(f, a_1, ..., a_{i - 1}, std::integral_constant<T, j_m>, a_{i + 1}, ..., a_n).
```

The return type of the main `invoke_intseq` call is
* `void` if `f` returns void,
* a type satisfying the `std::ranges::range` concept (aka a type we can iterate on) containing all results acquired as a result of applying `f` to the modified arguments in the order specified above.

## Additional requirements
1. We require perfect forwarding of both `f` and `args...`.
2. We require `constexpr`: `invoke_intseq` should be evaluated at compile time if all arguments (`f` and `args...`) are evaluated as such.

## Formal requirements
The solution will be compiled on the `students` machine with the following command:
```bash
clang++ -Wall -Wextra -std=c++20 -O2 *.cc
```

The solution has to pass the example test `invoke_intseq_exmpl.cc` (correct output in `invoke_intseq_exmpl.out`).