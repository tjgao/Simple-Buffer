#include <vector>
#include <typeinfo>
#include <iostream>
#include <map>
#include <array>
#include <set>
#include <iomanip>
#include <unordered_set>
#include "definitions.h"
#include "struct.h"
#include "buffer.h"


using namespace simple_buffer;

struct my_packet
{
	FIELD_START();
	FIELD(a, uint8_t);
	FIELD(b, uint32_t);
	FIELD(c, std::string);
	FIELD(d, std::array<int, 3>);
	ARRAY(e, [2][2], std::string);
	FIELD(f, std::map<std::string, std::vector<int>>);
	FIELD(g, std::tuple<std::string, std::pair<int, std::string>>);
	FIELD(h, double);
	FIELD(i, uint8_t);
	FIELD(j, std::unordered_set<std::string>)
	FIELD_END();

	void fill()
	{
		a = 0xff; i = 0xaa; b = 0x12345678; c = "Hello world", d = {123,456,789};
		e[0][0] = "Dolby"; e[0][1] = "COMM"; e[1][0] = "DVRTS"; e[1][1] = "AS3";
		f["Sydney"] = {0,1,2,3}; f["Melbourn"] = {3,2,1,0};
		std::get<0>(g) = "This is the tuple head";
		std::get<1>(g) = std::pair<int, std::string>(1111,"second elem of a pair");
		h = 1234.45678;
		j = {"Artarmon", "Campsie", "Chatswood", "Eastwood", "Lane Cove"};
	}

	void show()
	{
		std::cout << "a = " << std::hex << int(a) << std::endl;
		std::cout << "b = " << std::hex << int(b) << std::endl;
		std::cout << "c = " << c << std::endl;
		std::cout << "d = ";
		for (auto i : d) std::cout << std::dec << i << " ";
		std::cout << std::endl; 
		std::cout << "e = ";
		std::cout << e[0][0] << " " <<  e[0][1] << " " << e[1][0] << " " << e[1][1] << std::endl;
		std::cout << "f = " << std::endl;
		for (auto& a : f)
		{
			std::cout << a.first << ": ";
			for (auto& b : a.second) std::cout << b << " ";
			std::cout << std::endl;
		}
		std::setprecision(4);
		std::cout << "g = " << std::get<0>(g) << " | " << std::get<1>(g).first << ":" << std::get<1>(g).second << std::endl;
		std::cout << "h = " << std::fixed << h << std::endl;
		std::cout << "i = " << std::hex << int(i) << std::endl;
		std::cout << "j = " ; 
		for (auto& a : j) std::cout << a << " ";
		std::cout << std::endl;
	}
};

int main()
{
	auto_buf ab;

	my_packet mp1;
	mp1.fill();
	ab.write<my_packet>(mp1);

	my_packet mp2;
	ab.read<my_packet>(mp2);
	mp2.show();

}