# lazyVector

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/d1d79ecc1d9e48c18277f81b0cc0b76b)](https://www.codacy.com/app/DanIsraelMalta/lazyVector?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=DanIsraelMalta/lazyVector&amp;utm_campaign=Badge_Grade)

An implementation of std::vector (without costum allocator) such that all the avaialbe operators are lazy evaluated (via expression template proxies).
The only requirement from the user, is that the object held by the vector will have the required operators defined,
and that numerical/bit operators shall have both the regular (+,/,-,...) and the assignable (+=, /=, -=,....) form defined.

Other then lazy evaluation, lazy vector implements the exact api as std::vector.

the following shows that Lazy::Vector is between two to three times faster then std::vector (speed up factor changes with compiler).

```c  

std::vector<float> a0(100000, 1.0f), b0(100000, 2.0f), c0(100000, 3.0f), d0(100000, 4.0);
Lazy::Vector<float> a(100000, 1.0f), b(100000, 2.0f), c(100000, 3.0f), d(100000, 4.0);

auto start0{ std::chrono::high_resolution_clock::now() };
for (std::size_t i{}; i < 100000; ++i) {
    d0[i] -= (a0[i] * b0[i] + c0[i]) + (b0[i] / c0[i]) * (a0[i] / c0[i]);
}
auto end0{ std::chrono::high_resolution_clock::now() };
std::cout << " vector operation took " << std::chrono::duration_cast<std::chrono::microseconds>(end0 - start0).count() << ".\n";

auto start1{ std::chrono::high_resolution_clock::now() };
d -= (a + b + c) + (b / c) * (a / c);
auto end1{ std::chrono::high_resolution_clock::now() };
std::cout << " lazy operation took " << std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1).count() << ".\n";


std::cout << "d[0] = " << d[0] << ", d[1] = " << d[1];
```
