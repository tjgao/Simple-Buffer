#ifndef SIMPLE_BUFFER_CONDITIONS_DEF
#define SIMPLE_BUFFER_CONDITIONS_DEF

#include <type_traits>

namespace simple_buffer
{
typedef std::true_type yes;
typedef std::false_type no;

// Alignment size for struct field
template <typename T>
struct field_size
{
	static const size_t value = sizeof(T) + (sizeof(void*) - sizeof(T) % sizeof(void*)) % sizeof(void*);
};
// end for alignment size

// Condition for endian ops
template <typename T, bool E>
struct use_network_byteorder { static const bool value = E && std::is_arithmetic<T>::value && sizeof(T) > 1; };

template <typename T>
struct one_byte_type { static const bool value = sizeof(T) == 1; };
// end for endian ops

// For type_holder types
template <typename T>
struct is_type_holder
{
private:
	template <typename U> static auto has_type(int) -> decltype(std::declval<typename U::type_list>(), yes());
	template <typename U> static no has_type(...);
public:
	static const bool value = std::is_same<decltype(has_type<T>(0)), yes>::value;
};
// End for type_holder types

// For stl iterable container 
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
struct has_iterator_type
{
private:
	template <typename U> static auto test_iter(int) -> decltype(std::declval<typename U::iterator>(), yes());
	template <typename U> static no test_iter(...);
public:
	static constexpr bool value = std::is_same<decltype(test_iter<T>(0)), yes>::value;
};

template <typename T>
struct is_iterable_container
{
	const static bool value = has_begin_end<T>::value && has_iterator_type<T>::value;
};
// End for stl iterable container


// Network byte order operations for arithmetic types
template <typename T, size_t N>
struct endian_op {};

template <typename T>
struct endian_op<T, 2> 
{
	static T ntoh(T t) { return (T)ntohs(t); }
	static T hton(T t) { return (T)htons(t); }
};

template <typename T>
struct endian_op<T, 4> 
{
	static T ntoh(T t) { return (T)ntohl(t); }
	static T hton(T t) { return (T)htonl(t); }
};

template <typename T>
struct endian_op<T, 8> 
{
	static T ntoh(T t) { return (T)ntohll(t); }
	static T hton(T t) { return (T)htonll(t); }
};
// End for endian ops


// Remove head type from tuple
template <typename T>
struct remove_tuple_head{};

template <typename T, typename ...Ts>
struct remove_tuple_head<std::tuple<T, Ts...>> {  typedef std::tuple<Ts...> type; };
// end remove head for tuple
}
#endif // end of SIMPLE_BUFFER_CONDITIONS_DEF