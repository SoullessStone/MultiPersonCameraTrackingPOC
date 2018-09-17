#include <MainColorExtractor.h>
#include <iostream>

foo::foo(const std::string& s) 
  : _whatever(s) { }

void foo::print_whatever() const { 
    std::cout << _whatever; 
}
