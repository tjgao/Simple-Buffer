#ifndef SIMPLE_BUFFER_BASE_STRUCT_DEF
#define SIMPLE_BUFFER_BASE_STRUCT_DEF
#include <tuple>
#include "read_write.h"
#include "buffer.h"

#define TYPES(...) :type_holder<__VA_ARGS__> 
#define FIELD(type, name) struct alignas(sizeof(void*)){type name;}
namespace simple_buffer
{
template <typename ... ArgsT>
struct type_holder 
{ 
	typedef std::tuple<ArgsT...> type_list; 
	size_t size() { return rw<type_holder<ArgsT...>, true>::size(); }
	std::string str() 
	{ 
		buffer<> buf;
		buf.write<decltype(*this)>(*this);
		return buf.str();
	}
	buffer<>& read(buffer<>& buf) { buf.read<decltype(*this)>(*this); return buf;}
	buffer<>& write(buffer<>& buf) { buf.write<decltype(*this)>(*this); return buf; }
};
}
#endif //SIMPLE_BUFFER_BASE_STRUCT_DEF