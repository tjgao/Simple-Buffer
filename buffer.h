#ifndef SIMPLE_BUFFER_BUFFER_DEF
#define SIMPLE_BUFFER_BUFFER_DEF
#include <vector>
#include "struct.h"

namespace simple_buffer
{

class buffer_data
{
public:
	virtual size_t size() = 0;
	virtual char* data() = 0;
	virtual void resize(size_t sz) = 0;
	virtual bool resizable() { return false; }
};

class bytes_wrapper : public buffer_data
{
public:
	bytes_wrapper(){}
	bytes_wrapper(char* data, size_t len) : data_ptr(data), length(len) {} 
	virtual size_t size() override {  return length; }
	virtual char* data() override {  return data_ptr; }
	virtual void resize(size_t sz) override {};
private:
	char* data_ptr;
	size_t length;
};

class vector_wrapper : public buffer_data
{
public:
	vector_wrapper(){}
	virtual size_t size() override {  return vec.size(); }
	virtual void resize(size_t sz) override { vec.resize(sz); };
	virtual char* data() override { return vec.data(); }
	virtual bool resizable() override { return true; }
private:
	std::vector<char> vec;
};

template<typename U = vector_wrapper, bool CheckT = true, bool EndianT = true>
class buffer 
{
public:
	template<typename V = U, bool C = CheckT, bool E = EndianT>
	buffer(typename std::enable_if<std::is_same<V, vector_wrapper>::value, size_t>::type mem_grow_in = 1024) 
				: valid(true) 
				, cursor(0)
				, mem_grow(mem_grow_in)
	{
		local_buf.resize(mem_grow_in);
	}

	template<typename V = U, bool C = CheckT, bool E = EndianT>
	buffer(char* data_ptr, typename std::enable_if<std::is_same<V, bytes_wrapper>::value, size_t>::type length) 
				: valid(true)
				, cursor(0)
				, local_buf(data_ptr, length)
   {}

	char* data() { return local_buf.data(); }

	std::string str() { return std::string(local_buf.data(), cursor); }

	size_t size() { return cursor; }

	template <typename T>
	buffer& read(T& t) 
	{
		if (CheckT && rw<T, EndianT>::size(local_buf.data(), t) > local_buf.size())
		{
			// We don't have enough data to read
			valid = false;
			return *this;
		}
		cursor += rw<T, EndianT>::read(local_buf.data(), t);
		return *this;
	}


	template <typename T>
	buffer& write(const T& t) 
	{
		if (CheckT)
		{
			size_t sz = rw<T, EndianT>::size(local_buf.data(), t);
			if (sz > local_buf.size() && !local_buf.resizable())
			{
				valid = false;
				return *this;
			}
			for (; sz > local_buf.size(); inc_mem());
		}
		cursor += rw<T, EndianT>::write(local_buf.data(), t);
		return *this;
	}

	void reset() { valid = true; cursor = 0; }

	bool good() { return valid; }

	bool resizable() { return local_buf.resizable(); }
private:
	void inc_mem()
	{
	    local_buf.resize(local_buf.size() + mem_grow);
	}

	bool valid;
	U local_buf;
	size_t mem_grow, cursor;
};

typedef buffer<vector_wrapper, true, true> auto_buf;
typedef buffer<vector_wrapper, false, true> auto_nocheck_buf;
typedef buffer<vector_wrapper, false, false> auto_nocheck_noendian_buf;

typedef buffer<bytes_wrapper, true, true> fixed_buf;
typedef buffer<bytes_wrapper, false, true> fixed_nocheck_buf;
typedef buffer<bytes_wrapper, false, false> fixed_nocheck_noendian_buf;


}
#endif