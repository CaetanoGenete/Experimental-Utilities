﻿# Experimental Utilities

expu (Experimental-utilities) is a collection of useful tools and ideas I have built up over my stay in university, for
C++20. This
essentially came about after I discovered the wonderful world of c++ template meta programming and compile-time
programming.

## Features

### Template meta-programming set operation tools

A set of tools for dealing with type sets (For example std::tuple, but works with any type accepting variadic template arguments). Namely:

- union
- unique
- cartesian product
- contains

#### Examples

##### cartesian_product

```c++
#include <tuple>
#include "expu/meta/typelist_set_operations.hpp"

int main() {
    using tuple1 = std::tuple<int, float>;
    using tuple2 = std::tuple<double>;

    using prod_tuple = expu::cartesian_product_t<tuple1, tuple2>;
    // Equals: std::tuple<std::tuple<int, double>, std::tuple<float, double>>
}
```

##### unique

```c++
#include <tuple>
#include "expu/meta/typelist_set_operations.hpp"

int main() {
    using tuple = std::tuple<int, float, int, float, double>;

    using unique_tuple = expu::unique_t<tuple>;
    // Equals: std::tuple<int, float, double>
}
```

##### contains

```c++
#include <tuple>
#include <iostream>
#include "expu/meta/typelist_set_operations.hpp"

int main() {
    using tuple = std::tuple<int, float, int, float>;

    std::cout << expu::has_type_v<tuple, int> << std::endl;
    // prints true
    std::cout << expu::has_type_v<tuple, double> << std::endl;
    // prints false
}
```

### checked_allocator

A custom allocator that keeps track of memory it has allocated and de-allocated, throwing an exception if memory has not
been freed after its assigned container has been destroyed, or uninitialised memory has been accessed.

Effectively, this is a cheaper version of valgrind and Dr Memory, as it only provides diagnostics locally on the
container it is used. Naturally, said container must correctly implement the allocator_aware concept for it to be useful.

Example usage:

```c++
#include "expu/testing/checked_allocator.hpp"

int main()
{
    your_custom_container<int, checked_allocator<int>> vec;
}
```

### Custom iterators

#### seq_iter

An infinite integer iterator that increases

```c++
#include <vector>
#include "expu/iterators/seq_iter.hpp"

int main()
{
    std::vector vec(expu::seq_iter(0), expu::seq_iter(5));
    // Equals: {0, 1, 2, 3, 4}
}
```

#### Many more

Above were some of the more useful constructs (in my opinion), but there is a lot more to unpack here, from custom
data-structures to testing tools that can check if a certain type unexpectedly throws.

## Compiling

> [!IMPORTANT]
> The tests for this library have only been compiled on:
>
> - clang 18.1.6
> - MSVC 17
>
> It may or may not work on gcc or other compilers, beware!



expu is a header only library, copying the include folder into your project will .However if you wish to build and run the tests, this can be
achieved easily via cmake.

```shell
cmake -S . -B build -DEXPU_BUILD_TESTS=True
```
