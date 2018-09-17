#include <string>

class foo {
public:
    foo(const std::string& s);
    void print_whatever() const;
private:
    std::string _whatever;
};
