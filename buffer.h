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

template<bool EndianT = true>
class buffer 
{
public:
	buffer(bool check_size_in = true, size_t mem_grow_in = 1024) 
												: valid(true) 
												, cursor(0)
												, check_size(check_size_in)
												, mem_grow(mem_grow_in)
	{
		local_buf = &vw;
		local_buf->resize(mem_grow_in);
	}

	buffer(char* data_ptr, size_t length, bool check_size_in = true) 
											   : valid(true)
											   , cursor(0)
											   , check_size(check_size_in)
											   , bw(data_ptr, length)
   {
	   local_buf = &bw;
   }

	char* data() { return local_buf->data(); }

	std::string str() { return std::string(local_buf->data(), cursor); }

	size_t size() { return cursor; }

	template <typename T>
	size_t read(T& t) 
	{
		if (check_size && rw<T, EndianT>::size(local_buf->data(), t) > local_buf->size())
		{
			// We don't have enough data to read
			valid = false;
			return 0;
		}
		size_t sz = rw<T, EndianT>::read(local_buf->data(), t);
		cursor += sz;
		return sz;
	}

	template <typename T>
	size_t write(const T& t) 
	{
		if (check_size)
		{
			size_t sz = rw<T, EndianT>::size(local_buf->data(), t);
			if (sz > local_buf->size() && !local_buf->resizable())
			{
				valid = false;
				return 0;
			}
			for (; sz > local_buf->size(); inc_mem());
		}
		size_t sz = rw<T, EndianT>::write(local_buf->data(), t);
		cursor += sz;
		return sz;
	}

	void reset() { valid = true; cursor = 0; }

	bool good() { return valid; }

	bool resizable() { return local_buf->resizable(); }
private:
	void inc_mem()
	{
	    local_buf->resize(local_buf->size() + mem_grow);
	}

	bool valid, check_size;
	bytes_wrapper bw;
	vector_wrapper vw;
	buffer_data* local_buf;
	size_t mem_grow, cursor;
};

}
#endif