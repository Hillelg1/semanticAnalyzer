#include <iostream>
#include <variant>
#include <string>

int main() {
	std::variant<int, double, std::string> v = "hello";
	std::visit([](const auto& x) {
		std::cout << x << std::endl;
	}, v);
}