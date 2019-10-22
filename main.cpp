#include <iostream>

extern "C" int add(int x, int y);

int main() {
    std::cout << add(12, 14) << std::endl;
}