#ifndef SIMPLE_BUFFER_BASE_STRUCT_DEF
#define SIMPLE_BUFFER_BASE_STRUCT_DEF
#include <tuple>
#include "read_write.h"
#include "buffer.h"

namespace simple_buffer
{

#define FIELD_START()																													\
struct sentinel{};																														\
template <size_t N, bool dummy>	struct field_mark{ typedef void type; };           														\
template<bool dummy> struct field_mark<__LINE__, dummy>{ typedef sentinel type;};														\


#define FIELD(name, ...) 																												\
alignas(sizeof(void*)) __VA_ARGS__ name;                              																	\
template <bool dummy> struct field_mark<__LINE__, dummy> { typedef __VA_ARGS__ type; };

#define ARRAY(name, extent, ...)																										\
alignas(sizeof(void*)) __VA_ARGS__ name extent;																							\
template <bool dummy> struct field_mark<__LINE__, dummy> { typedef decltype(name) type; };


#define FIELD_END()                                                                                                         			\
template <bool dummy, size_t N, typename T, typename ...Args>																			\
struct field_collector{};																												\
template <bool dummy, size_t N, typename T, typename ...Args>																			\
struct field_collector<dummy, N, T, std::tuple<Args...>>																				\
{																																		\
	typedef typename field_collector<dummy, N-1, typename field_mark<N-1, dummy>::type, std::tuple<T, Args...>>::type type;				\
};																																		\
template <bool dummy, size_t N, typename ...Args>																						\
struct field_collector<dummy, N, void, std::tuple<Args...>>																				\
{																																		\
	typedef typename field_collector<dummy, N-1, typename field_mark<N-1, dummy>::type, std::tuple<Args...>>::type type;				\
};																																		\
template <bool dummy, size_t N, typename ...Args>																						\
struct field_collector<dummy, N, sentinel, std::tuple<Args...>>																			\
{																																		\
	typedef std::tuple<Args...> type;																									\
};																																		\
typedef int _trust_me_i_am_your_type_;																									\
typedef field_collector<true, __LINE__, void, std::tuple<>>::type type_list;															\
std::string str() { auto_buf ab; return ab.write(*this).str();  }																		\

}
#endif //SIMPLE_BUFFER_BASE_STRUCT_DEF