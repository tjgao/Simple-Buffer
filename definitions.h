#ifndef SIMPLE_BUFFER_CONDITIONS_DEF
#define SIMPLE_BUFFER_CONDITIONS_DEF

#include <type_traits>

namespace simple_buffer
{
typedef std::true_type yes;
typedef std::false_type no;

// Alignment size for struct field
template <typename T>
struct field_aligned_size
{
	static const size_t value = sizeof(T) + (sizeof(void*) - sizeof(T) % sizeof(void*)) % sizeof(void*);
};
// end for alignment size

// Condition for endian ops
template <typename T, bool E>
struct use_network_byteorder { static const bool value = E && std::is_arithmetic<T>::value && sizeof(T) > 1; };
// end for endian ops

// To identify our serializable struct
template <typename T>
struct is_serializable_struct
{
private:
	template <typename U> static auto has_type(int) -> decltype(std::declval<typename U::_trust_me_i_am_your_type_>(), yes());
	template <typename U> static no has_type(...);
public:
	static const bool value = std::is_same<decltype(has_type<T>(0)), yes>::value;
};
// end for our serializable struct

// For stl iterable and modifable container 
template <typename T>
struct has_begin_end
{
private:
	template <typename U> static auto has_begin(int) -> decltype(std::declval<U>().begin(), yes());
	template <typename U> static no has_begin(...);
	template <typename U> static auto has_end(int) -> decltype(std::declval<U>().end(), yes());
	template <typename U> static no has_end(...);
public:
	static constexpr bool value = std::is_same<decltype(has_begin<T>(0)), yes>::value
								&& std::is_same<decltype(has_end<T>(0)), yes>::value;
};

template <typename T>
struct has_iterator_clear
{
private:
	template <typename U> static auto test_iter(int) -> decltype(std::declval<typename U::iterator>(), yes());
	template <typename U> static no test_iter(...);
	template <typename U> static auto has_clear(int) -> decltype(std::declval<U>().clear(), yes());
	template <typename U> static no has_clear(...);
public:
	static constexpr bool value = std::is_same<decltype(test_iter<T>(0)), yes>::value
								&& std::is_same<decltype(has_clear<T>(0)), yes>::value;
};

template <typename T>
struct is_modifiable_container
{
	const static bool value = has_begin_end<T>::value && has_iterator_clear<T>::value;
};
// End for stl iterable and modifiable container


// Network byte order operations for arithmetic types
template <size_t N>
struct matched_uint {};

template <>
struct matched_uint<1> { typedef uint8_t type; };

template <>
struct matched_uint<2> { typedef uint16_t type; };

template <>
struct matched_uint<4> { typedef uint32_t type; };

template <>
struct matched_uint<8> { typedef uint64_t type; };

template <typename T, size_t N>
struct endian_op {};

template <typename T>
struct endian_op<T, 2> 
{
	typedef typename matched_uint<2>::type int_type;
	static T ntoh(int_type t) { t = ntohs(t); T* p = (T*)&t; return *p; }
	static int_type hton(T t) { int_type* p = (int_type*)&t; return (int_type)htons(*p); }
};

template <typename T>
struct endian_op<T, 4> 
{
	typedef typename matched_uint<4>::type int_type;
	static T ntoh(int_type t) { t = ntohl(t); T *p = (T*)&t; return *p; }
	static int_type hton(T t) { int_type* p = (int_type*)&t; return (int_type)htonl(*p); }
};

template <typename T>
struct endian_op<T, 8> 
{
	typedef typename matched_uint<8>::type int_type;
	static T ntoh(int_type t) { t = ntohll(t); T *p = (T*)&t; return *p; }
	static int_type hton(T t) { int_type* p = (int_type*)&t; return (int_type)htonll(*p); }
};
// End for endian ops


// Remove head type from tuple
template <typename T>
struct remove_tuple_head{ typedef void type; };

template <typename T, typename ...Ts>
struct remove_tuple_head<std::tuple<T, Ts...>> 
{  
	typedef typename std::conditional<(std::tuple_size<std::tuple<Ts...>>::value > 0), std::tuple<Ts...>, std::tuple<>>::type type; 
};
// end remove head for tuple
}
#endif // end of SIMPLE_BUFFER_CONDITIONS_DEF