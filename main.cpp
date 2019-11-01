#include <iostream>

extern "C" int add(int x, int y);

int main() {
    std::cout << add(10, -1) << std::endl;
}
