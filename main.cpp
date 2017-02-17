#include <vector>
#include <typeinfo>
#include <iostream>
#include <map>
#include <array>
#include <set>
#include <iomanip>
#include <list>
#include <unordered_set>
#include "struct.h"
#include "buffer.h"


using namespace simple_buffer;

/*
Support STL containers, vector, map, set, list, tuple, pair, array, deque
std::string
fundamental types and raw arrays (without limit to the extent and rank)
and our serializable structs equipped with FIELD macros (there is no difference between an integer 
type and our serializable struct type)

Currently pointers are not supported. It is possible, but can be much more compilicated, 
as pointers can form loops and they can point to the same object

Can support user defined types if the corresponding read/write code is provided.

Normal members can be added after the FIELD_END macro, but they will not be serialized/deserialized. 
 
*/

struct my_packet
{
	struct inner_packet
	{
		FIELD_START();
		FIELD(x, int);
		FIELD(y, std::list<int>);
		ARRAY(z, [5], char);
		FIELD_END();
	};
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
	FIELD(j, inner_packet);
	FIELD(k, std::unordered_set<std::string>);
	FIELD_END();

	void fill()
	{
		a = 0xff; i = 0xaa; b = 0x12345678; c = "Hello world", d = {123,456,789};
		e[0][0] = "Dolby"; e[0][1] = "COMM"; e[1][0] = "DVRTS"; e[1][1] = "AS3";
		f["Sydney"] = {0,1,2,3}; f["Melbourn"] = {3,2,1,0};
		std::get<0>(g) = "This is the tuple head";
		std::get<1>(g) = std::pair<int, std::string>(1111,"second elem of a pair");
		h = 1234.45678;
		j = {999, {1,2,3,4,5,6}, {'a','b','c','d'}};
		k = {"Artarmon", "Campsie", "Chatswood", "Eastwood", "Lane Cove"};
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
		std::cout << "j.x = " << std::dec << j.x << std::endl;
		std::cout << "j.y = ";
		for (auto& el : j.y) std::cout << el << " ";
		std::cout << std::endl; 
		std::cout << "j.z = " << " ";
		for (int idx = 0; idx < sizeof(j.z); idx++) std::cout << j.z[idx] << " ";
		std::cout << std::endl;
		std::cout << "k = " ; 
		for (auto& a : k) std::cout << a << " ";
		std::cout << std::endl;
	}
};

int main()
{
	auto_buf ab;

	my_packet mp1;
	mp1.fill();
	ab.write(mp1);

	my_packet mp2;
	ab.read(mp2);
	mp2.show();

	std::cout <<"------ " << std::endl;
	//Or just some normal containers
	std::vector<std::string> vs{"vector", "string", "tuple"};
	ab.write(vs);
	decltype(vs) vvss;
	ab.read(vvss);
	for (auto& s : vvss) std::cout << s << " ";
	std::cout << std::endl;
}