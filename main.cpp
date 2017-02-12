#include <vector>
#include <typeinfo>
#include <iostream>
#include <array>
#include "struct.h"
#include "buffer.h"


using namespace simple_buffer;

struct my_packet
TYPES(uint8_t, std::string, uint8_t, float, std::vector<std::string>)
{
	FIELD(uint8_t, type);
	FIELD(std::string, name);
	FIELD(uint8_t, age);
	FIELD(float, money);
	FIELD(std::vector<std::string>, shops);
};

int main()
{
	buffer<> buf;
	my_packet ab;
	ab.type = 1;
	ab.name = "James Bond";
	ab.age = 45;
	ab.money = 2929292;
	ab.shops = {"Pizza Hut", "Hungry Jacks", "JB HIFI"};

	std::cout << ab.write(buf).size() << std::endl;
}