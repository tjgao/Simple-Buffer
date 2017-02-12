#ifndef SIMPLE_BUFFER_READ_WRITE_DEF
#define SIMPLE_BUFFER_READ_WRITE_DEF
#include <type_traits>
#include "definitions.h"

namespace simple_buffer
{

template <typename T, bool E, typename Enabled = void>
struct rw{};

// For type_holder type
template <typename T, bool E>
struct rw<T, E, typename std::enable_if<is_type_holder<T>::value, T>::type>
{
public:
	static size_t read(const char* data, T& t)
	{
		return rw<typename T::type_list, E>::read(data, reinterpret_cast<char*>(&t));
	}

	static size_t write(char* data, T& t)
	{
		return rw<typename T::type_list, E>::write(data, reinterpret_cast<const char*>(&t));
	}

	static size_t size(const char* data, const T& t)
	{
		return rw<typename T::type_list, E>::size(data, reinterpret_cast<const char*>(&t));
	}

private:
	template <typename U>
	static size_t read(const char* data, char* obj)
	{
		typedef typename std::tuple_element<0, U>::type HeadType; 
		HeadType& t = *(reinterpret_cast<HeadType*>(obj));
		size_t sz = rw<HeadType, E>::read(data, t);
		data += sz;
		obj += field_size<HeadType>::value;
		return std::tuple_size<U>::value <= 1 ? sz :
		sz + rw<typename remove_tuple_head<U>::type, E>::read(data, obj);
	}

	template <typename U>
	static size_t write(char* data, const char* obj)
	{
		typedef typename std::tuple_element<0, U>::type HeadType;
		HeadType& t = *(reinterpret_cast<HeadType*>(obj));
		size_t sz = rw<HeadType, E>::write(data, t);
		data += sz;
		obj += field_size<HeadType>::value;
		return std::tuple_size<U>::value <= 1 ? sz :
		sz + rw<typename remove_tuple_head<U>::type, E>::write(data, obj);
	}
	template <typename U>
	static size_t size(const char* data, const char* obj)
	{
		typedef typename std::tuple_element<0, U>::type HeadType;
		const HeadType& t = *(reinterpret_cast<const HeadType*>(obj));
		size_t sz = rw<HeadType, E>::size(data, t);
		data += sz;
		obj += field_size<HeadType>::value;
		return std::tuple_size<U>::value <= 1 ? sz :
		sz + rw<typename remove_tuple_head<U>::type, E>::size(data, obj);
	}
};
// End for type_holder types

// For arithmetic types
template <typename T, bool E>
struct rw<T, E, typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
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
	// read in network byte order
	static size_t read_impl(const char* data, T& t, std::true_type)
	{ t = endian_op<T, sizeof(T)>::ntoh(*((const T*)data)); return sizeof(T); }
	// just read it
	static size_t read_impl(const char* data, T& t, std::false_type)
	{ t = *((T*)data); return sizeof(T); }

	// write in network byte order
	static size_t write_impl(char* data, const T& t, std::true_type)
	{ *((T*)data) = endian_op<T, sizeof(T)>::hton(t); return sizeof(T); }
	// just write it
	static size_t write_impl(char* data, const T& t, std::false_type)
	{ *((T*)data) = t; return sizeof(T); }
};

// Specialization for std::string
template <bool E>
struct rw<std::string, E>
{
	static size_t read(const char* data, std::string& t)
	{
		uint32_t str_len = 0, size = sizeof(uint32_t);
		rw<uint32_t, E>::read(data, str_len);
		data += size;
		t = std::string(data, str_len); 
		size += str_len;
		return size;
	}

	static size_t write(char* data, const std::string& t)
	{
		rw<uint32_t, E>::write(data, (uint32_t)t.size());
		data += sizeof(uint32_t);
		memcpy(data, t.data(), t.size());
		return sizeof(uint32_t) + t.size();
	}

	static size_t size(const char* data, const std::string& t)
	{ return t.size() + sizeof(uint32_t); }
}; 

// For iterable containers, such as vector, map, set, etc
template <typename T, bool E>
struct rw<T, E, typename std::enable_if<is_iterable_container<T>::value, T>::type>
{
	static size_t read(const char* data, T& t)
	{
		uint32_t size = 0;
		const char* old = data;
		data += rw<uint32_t, E>::read(data, size);
		for (uint32_t i = 0; i < size; i++)
		{
			typename T::value_type elem;
			data += rw<typename T::value_type, E>::read(data, elem);
			t.insert(t.end(), std::move(elem));	
		}
		return data - old;
	}

	static size_t write(char* data, const T& t)
	{
		const char* old = data;
		data += rw<uint32_t, E>::write(data, t.size());
		for (auto& i : t)
			data += rw<typename T::value_type, E>::write(data, i);
		return data - old;
	}

	static size_t size(const char* data, const T& t) 
	{
		size_t sz = 0;
		for (auto& i : t)
			sz += rw<typename T::value_type, E>::size(data, i);
		return sz + sizeof(uint32_t);
	}
};

// For raw arrays
template <typename T, bool E>
struct rw<T, E, typename std::enable_if<std::is_array<T>::value, T>::type>
{
	static size_t read(const char* data, T& t)
	{
		const char* old = data;
		for (size_t i = 0; i < std::extent<T, 0>::value; i++)
			data += rw<decltype(t[0]), E>::read(data, t[i]);
		return data - old;
	}
	static size_t write(char* data, const T& t)
	{
		char* old = data;
		for (size_t i = 0; i < std::extent<T, 0>::value; i++)
			data += rw<decltype(t[0]), E>::write(data, t[i]);
		return data - old;
	}
	static size_t size(const char* data, const T& t)
	{
		size_t sz = 0;
		for (size_t i = 0; i < std::extent<T, 0>::value; i++)
			sz += rw<decltype(t[0]), E>::size(data, t[i]);
		return sz;
	}
};
// End raw arrays

// Specialization for std::array
template <typename T, size_t N, bool E>
struct rw<std::array<T, N>, E>
{
	static size_t read(const char* data, std::array<T, N>& t)
	{
		const char* old = data;
		for (size_t i = 0; i < N; i++)
			data += rw<typename std::array<T, N>::value_type, E>::read(data, t[i]);
		return data - old;
	}

	static size_t write(char* data, const std::array<T, N>& t)
	{
		char* old = data;
		for (size_t i = 0; i < N; i++)
			data += rw<typename std::array<T, N>::value_type, E>::write(data, t[i]);
		return data - old;
	}

	static size_t size(const char* data, const std::array<T, N>& t)
	{
		size_t sz = 0;
		for (size_t i = 0; i < N; i++)
			sz += rw<typename std::array<T,N>::value_type, E>::size(data, t[i]);
		return sz;
	}
};

}

#endif // end of SIMPLE_BUFFER_READ_WRITE_DEF