#ifndef SIMPLE_BUFFER_READ_WRITE_DEF
#define SIMPLE_BUFFER_READ_WRITE_DEF
#include <type_traits>
#include "definitions.h"
#include <typeinfo>
namespace simple_buffer
{


template <typename T, bool E, typename TagT = void>
struct rw_worker{};

// For serializable struct type
template <typename T, bool E>
struct rw_worker<T, E, typename std::enable_if<is_serializable_struct<T>::value, T>::type>
{
public:
	typedef typename std::conditional<(std::tuple_size<typename T::type_list>::value > 0), yes, no>::type bool_type;
	static size_t read(const char* data, T& t)
	{
		return read<typename T::type_list>(data, reinterpret_cast<char*>(&t), bool_type());
	}

	static size_t write(char* data, const T& t)
	{
		return write<typename T::type_list>(data, reinterpret_cast<const char*>(&t), bool_type());
	}

	static size_t size(const char* data, const T& t)
	{
		return size<typename T::type_list>(data, reinterpret_cast<const char*>(&t), bool_type());
	}

private:
	template <typename U>
	static size_t read(const char* data, char* obj, no) { return 0; }

	template <typename U>
	static size_t write(char* data, const char* obj, no) { return 0; }

	template <typename U>
	static size_t size(const char* data, const char* obj, no) { return 0; }

	template <typename U>
	static size_t read(const char* data, char* obj, yes)
	{
		typedef typename std::tuple_element<0, U>::type HeadType; 
		HeadType& t = *(reinterpret_cast<HeadType*>(obj));
		size_t sz = rw_worker<HeadType, E, HeadType>::read(data, t);
		data += sz;
		obj += field_aligned_size<HeadType>::value;
		typedef typename remove_tuple_head<U>::type tail;
		typedef typename std::conditional<(std::tuple_size<tail>::value > 0), yes, no>::type bool_type;
		return sz + read<tail>(data, obj, bool_type());
	}

	template <typename U>
	static size_t write(char* data, const char* obj, yes)
	{
		typedef typename std::tuple_element<0, U>::type HeadType;
		const HeadType& t = *(reinterpret_cast<const HeadType*>(obj));
		size_t sz = rw_worker<HeadType, E, HeadType>::write(data, t);
		data += sz;
		obj += field_aligned_size<HeadType>::value;
		typedef typename remove_tuple_head<U>::type tail;
		typedef typename std::conditional<(std::tuple_size<tail>::value > 0), yes, no>::type bool_type;
		return sz + write<tail>(data, obj, bool_type());
	}

	template <typename U>
	static size_t size(const char* data, const char* obj, yes)
	{
		typedef typename std::tuple_element<0, U>::type HeadType;
		const HeadType& t = *(reinterpret_cast<const HeadType*>(obj));
		size_t sz = rw_worker<HeadType, E, HeadType>::size(data, t);
		data += sz;
		obj += field_aligned_size<HeadType>::value;
		typedef typename remove_tuple_head<U>::type tail;
		typedef typename std::conditional<(std::tuple_size<tail>::value > 0), yes, no>::type bool_type;
		return sz + size<tail>(data, obj, bool_type());
	}
};
// End for serializable struct type

// For arithmetic types
template <typename T, bool E>
struct rw_worker<T, E, typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
{
public:
	static size_t read(const char* data, T& t)
	{ 
		typedef typename std::conditional<(E && sizeof(T) > 1), yes, no>::type bool_type;
		return read_impl(data, t, bool_type());
	}

	static size_t write(char* data, const T& t)
	{ 
		typedef typename std::conditional<(E && sizeof(T) > 1), yes, no>::type bool_type;
		return write_impl(data, t, bool_type());
	}

	static size_t size(const char* data, const T& t)
	{ return sizeof(T); }
private:
	typedef typename matched_uint<sizeof(T)>::type int_type;
	// read in network byte order
	static size_t read_impl(const char* data, T& t, std::true_type)
	{ 
		t = endian_op<T, sizeof(T)>::ntoh(*((const int_type*)data)); 
		return sizeof(T); 
	}
	// just read it
	static size_t read_impl(const char* data, T& t, std::false_type)
	{ t = *((T*)data); return sizeof(T); }

	// write in network byte order
	static size_t write_impl(char* data, const T& t, std::true_type)
	{ int_type i = endian_op<T, sizeof(T)>::hton(t); memcpy(data, &i, sizeof(T)); return sizeof(T); }
	// just write it
	static size_t write_impl(char* data, const T& t, std::false_type)
	{ *((T*)data) = t; return sizeof(T); }
};

// Specialization for std::string
template <bool E>
struct rw_worker<std::string, E, std::string>
{
	static size_t read(const char* data, std::string& t)
	{
		uint32_t str_len = 0, size = sizeof(uint32_t);
		rw_worker<uint32_t, E, uint32_t>::read(data, str_len);
		data += size;
		t = std::string(data, str_len); 
		size += str_len;
		return size;
	}

	static size_t write(char* data, const std::string& t)
	{
		rw_worker<uint32_t, E, uint32_t>::write(data, (uint32_t)t.size());
		data += sizeof(uint32_t);
		memcpy(data, t.data(), t.size());
		return sizeof(uint32_t) + t.size();
	}

	static size_t size(const char* data, const std::string& t)
	{ return t.size() + sizeof(uint32_t); }
}; 
// End std::string

// For iterable and modifiable containers, such as vector, map, set, list, etc
template <typename T, bool E>
struct rw_worker<T, E, typename std::enable_if<is_modifiable_container<T>::value, T>::type>
{
	static size_t read(const char* data, T& t)
	{
		uint32_t size = 0;
		const char* old = data;
		data += rw_worker<uint32_t, E, uint32_t>::read(data, size);
		for (uint32_t i = 0; i < size; i++)
		{
			typename T::value_type elem;
			data += rw_worker<typename T::value_type, E, typename T::value_type>::read(data, elem);
			t.insert(t.end(), std::move(elem));	
		}
		return data - old;
	}

	static size_t write(char* data, const T& t)
	{
		const char* old = data;
		data += rw_worker<uint32_t, E, uint32_t>::write(data, t.size());
		for (auto& i : t)
			data += rw_worker<typename T::value_type, E, typename T::value_type>::write(data, i);
		return data - old;
	}

	static size_t size(const char* data, const T& t) 
	{
		const char* old = data;
		for (auto& i : t)
			data += rw_worker<typename T::value_type, E, typename T::value_type>::size(data, i);
		return data - old;
	}
};
// End stl containers

// For raw arrays
template <typename T, bool E>
struct rw_worker<T, E, typename std::enable_if<std::is_array<T>::value, T>::type>
{
	static size_t read(const char* data, T& t)
	{
		typedef typename std::remove_cv<typename std::remove_reference<decltype(t[0])>::type>::type subtype;
		const char* old = data;
		for (size_t i = 0; i < std::extent<T, 0>::value; i++)
			data += rw_worker<subtype, E, subtype>::read(data, t[i]);
		return data - old;
	}
	static size_t write(char* data, const T& t)
	{
		typedef typename std::remove_cv<typename std::remove_reference<decltype(t[0])>::type>::type subtype;
		char* old = data;
		for (size_t i = 0; i < std::extent<T, 0>::value; i++)
			data += rw_worker<subtype, E, subtype>::write(data, t[i]);
		return data - old;
	}
	static size_t size(const char* data, const T& t)
	{
		typedef typename std::remove_cv<typename std::remove_reference<decltype(t[0])>::type>::type subtype;
		size_t sz = 0;
		for (size_t i = 0; i < std::extent<T, 0>::value; i++)
			sz += rw_worker<subtype, E, subtype>::size(data, t[i]);
		return sz;
	}
};
// End raw arrays

// Specialization for std::array
template <typename T, size_t N, bool E>
struct rw_worker<std::array<T, N>, E, std::array<T, N>>
{
	typedef typename std::array<T, N>::value_type elem_type;
	static size_t read(const char* data, std::array<T, N>& t)
	{
		const char* old = data;
		for (size_t i = 0; i < N; i++)
			data += rw_worker<elem_type, E, elem_type>::read(data, t[i]);
		return data - old;
	}

	static size_t write(char* data, const std::array<T, N>& t)
	{
		char* old = data;
		for (size_t i = 0; i < N; i++)
			data += rw_worker<elem_type, E, elem_type>::write(data, t[i]);
		return data - old;
	}

	static size_t size(const char* data, const std::array<T, N>& t)
	{
		size_t sz = 0;
		for (size_t i = 0; i < N; i++)
			sz += rw_worker<elem_type, E, elem_type>::size(data, t[i]);
		return sz;
	}
};
// End std::array

// For std::pair
template <typename U, typename V, bool E>
struct rw_worker<std::pair<U, V>, E, std::pair<U, V>>
{
	static size_t read(const char* data, std::pair<U, V>& t)
	{
		typedef typename std::remove_cv<U>::type FirstT; 
		typedef typename std::remove_cv<V>::type SecondT; 
		const char* old = data;
		data += rw_worker<FirstT, E, FirstT>::read(data, (FirstT&)t.first);
		data += rw_worker<SecondT, E, SecondT>::read(data, t.second);
		return data - old;
	}
	static size_t write(char* data, const std::pair<U, V>& t)
	{
		typedef typename std::remove_cv<U>::type FirstT; 
		typedef typename std::remove_cv<V>::type SecondT; 
		const char* old = data;
		data += rw_worker<FirstT, E, FirstT>::write(data, t.first);
		data += rw_worker<SecondT, E, SecondT>::write(data, t.second);
		return data - old;
	}
	static size_t size(const char* data, const std::pair<U,V>& t)
	{
		typedef typename std::remove_cv<U>::type FirstT; 
		typedef typename std::remove_cv<V>::type SecondT; 
		const char* old = data;
		data += rw_worker<FirstT, E, FirstT>::size(data, t.first);
		data += rw_worker<SecondT, E, SecondT>::size(data, t.second);
		return data - old;
	}
};
// End std::pair


// For std::tuple
template <typename T, bool E>
struct rw_worker<T, E, typename std::enable_if<!std::is_void<typename remove_tuple_head<T>::type>::value, T>::type>
{
public:
	static size_t read(const char* data, T& t)
	{
		typedef typename std::conditional<(std::tuple_size<T>::value > 0), yes, no>::type bool_type;
		return read<T, 0>(data, t, bool_type());
	}
	static size_t write(char* data, const T& t)
	{
		typedef typename std::conditional<(std::tuple_size<T>::value > 0), yes, no>::type bool_type;
		return write<T, 0>(data, t, bool_type());
	}
	static size_t size(const char* data, const T& t)
	{
		typedef typename std::conditional<(std::tuple_size<T>::value > 0), yes, no>::type bool_type;
		return size<T, 0>(data, t, bool_type());
	}
private:
	template <typename TP, size_t N>
	static size_t read(const char* data, TP& t, no) { return 0; }
	template<typename TP, size_t N>
	static size_t read(const char* data, TP& t, yes)
	{
		auto& elem = std::get<N>(t);
		typedef typename std::tuple_element<N, TP>::type elem_type;
		const char* old = data;
		data += rw_worker<elem_type, E, elem_type>::read(data, elem);
		typedef typename std::conditional<(std::tuple_size<TP>::value > N+1), yes, no>::type bool_type;
		return data - old + read<TP, N+1>(data, t, bool_type());
	}

	template <typename TP, size_t N>
	static size_t write(char* data, const TP& t, no) { return 0; }
	template<typename TP, size_t N>
	static size_t write(char* data, const TP& t, yes)
	{
		auto& elem = std::get<N>(t);
		typedef typename std::tuple_element<N, TP>::type elem_type;
		char* old = data;
		data += rw_worker<elem_type, E, elem_type>::write(data, elem);
		typedef typename std::conditional<(std::tuple_size<TP>::value > N+1), yes, no>::type bool_type;
		return data - old + write<TP, N+1>(data, t, bool_type());
	}

	template <typename TP, size_t N>
	static size_t size(const char* data, const TP& t, no) { return 0; }
	template<typename TP, size_t N>
	static size_t size(const char* data, const TP& t, yes)
	{
		auto& elem = std::get<N>(t);
		typedef typename std::tuple_element<N, TP>::type elem_type;
		const char* old = data;
		data += rw_worker<elem_type, E, elem_type>::size(data, elem);
		typedef typename std::conditional<(std::tuple_size<TP>::value > N+1), yes, no>::type bool_type;
		return data - old + size<TP, N+1>(data, t, bool_type());
	}
};
// End std::tuple

template <typename T, bool E>
struct rw
{
	static size_t read(const char* data, T& t) { return rw_worker<T, E, T>::read(data, t); }
	static size_t write(char* data, const T& t) { return rw_worker<T, E, T>::write(data, t); }
	static size_t size(const char* data, const T& t) { return rw_worker<T, E, T>::size(data, t); }
};

}

#endif // end of SIMPLE_BUFFER_READ_WRITE_DEF