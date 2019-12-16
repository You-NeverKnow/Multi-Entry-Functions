#include <iostream>

extern "C" int inc(int x);

__attribute__((thiscall))
int main() {
    std::cout << inc(10) << std::endl;
}
