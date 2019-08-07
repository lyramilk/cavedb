#ifndef _lyramilk_data_defs_h_
#define _lyramilk_data_defs_h_

#include "config.h"
#include <map>
#include <vector>
#include <stdio.h>

#ifdef Z_HAVE_UNORDEREDMAP
	#include <unordered_map>
	#define HAVE_UNORDEREDMAP
#elif defined Z_HAVE_TR1_UNORDEREDMAP
	#include <tr1/unordered_map>
	#define HAVE_UNORDEREDMAP
#endif

/**
	@namespace lyramilk::data
	@brief 数据
	@details 该命名空间描述数据的表达形式。
*/


//		typedef lyramilk::data::case_insensitive_unordered_map<lyramilk::data::string,lyramilk::data::string> stringdict;

namespace lyramilk{namespace data
{
#ifdef Z_HAVE_UNORDEREDMAP
	using std::unordered_map;
	using std::hash;
#elif defined Z_HAVE_TR1_UNORDEREDMAP
	using std::tr1::unordered_map;
	using std::tr1::hash;
#endif

	typedef char int8;
	typedef unsigned char uint8;

	typedef short int16;
	typedef unsigned short uint16;

	typedef int int32;
	typedef unsigned int uint32;

	typedef long long int64;
	typedef unsigned long long uint64;


	_lyramilk_api_ void* milk_malloc(size_t size);
	_lyramilk_api_ void milk_free(void* p, size_t size);

#ifdef USEMILKALLOC
	/**
		@brief 这是一个支持标准库的内存分配器，可用于标准库中各种容器对象的安全跨库使用。
	*/
	template <typename T>
	class allocator
	{
	  public:
		typedef size_t		size_type;
		typedef ptrdiff_t	difference_type;
		typedef T*			pointer;
		typedef const T*	const_pointer;
		typedef T&			reference;
		typedef const T&	const_reference;
		typedef T			value_type;

		template<typename U>
		struct rebind {
			typedef allocator<U> other;
		};
		allocator() throw()
		{}
		allocator(const allocator&) throw()
		{}
		template<typename U>
		allocator(const allocator<U>&) throw()
		{}
		~allocator() throw()
		{}

		pointer address(reference __x) const { return &__x; }
		const_pointer address(const_reference __x) const { return &__x; }

		pointer allocate(size_type __n, const void* = 0)
		{
			return static_cast<T*>(lyramilk::data::milk_malloc(__n * sizeof(T)));
		}
		void deallocate(pointer __p, size_type __n)
		{
			lyramilk::data::milk_free(__p,__n * sizeof(T));
		}
		size_type max_size() const throw()
		{
			return size_t(-1) / sizeof(T);
		}

		void construct(pointer __p, const T& __val)
		{
			::new((void *)__p) T(__val);
		}

#if __cplusplus >= 201103L
		template<typename... _Args> void construct(pointer __p, _Args&&... __args)
		{
			::new((void *)__p) T(__args...);
		}
#endif
		void destroy(pointer __p)
		{
#ifdef _MSC_VER
			__p;
#endif
			__p->~T();
		}
	};

	template <typename T> inline bool operator==(const allocator<T>&, const allocator<T>&) { return true; }
	template <typename T> inline bool operator!=(const allocator<T>&, const allocator<T>&) { return false; }

	template <typename T> inline bool operator==(const allocator<T>&, const std::allocator<T>&) { return true; }
	template <typename T> inline bool operator!=(const allocator<T>&, const std::allocator<T>&) { return false; }
#else
	using std::allocator;
#endif


	typedef class _lyramilk_api_ std::basic_string<char, std::char_traits<char>, allocator<char> > string;
	typedef class _lyramilk_api_ std::basic_string<unsigned char, std::char_traits<unsigned char>, allocator<unsigned char> > chunk;
	typedef class _lyramilk_api_ std::basic_string<wchar_t, std::char_traits<wchar_t>, allocator<wchar_t> > wstring;

	typedef class _lyramilk_api_ std::basic_stringstream<char, std::char_traits<char>, allocator<char> > stringstream;
	typedef class _lyramilk_api_ std::basic_stringstream<unsigned char, std::char_traits<unsigned char>, allocator<unsigned char> > datastream;
	typedef class _lyramilk_api_ std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, allocator<wchar_t> > wstringstream;

	typedef class _lyramilk_api_ std::basic_istringstream<char, std::char_traits<char>, allocator<char> > istringstream;
	typedef class _lyramilk_api_ std::basic_istringstream<unsigned char, std::char_traits<unsigned char>, allocator<unsigned char> > idatastream;
	typedef class _lyramilk_api_ std::basic_istringstream<wchar_t, std::char_traits<wchar_t>, allocator<wchar_t> > wistringstream;

	typedef class _lyramilk_api_ std::basic_ostringstream<char, std::char_traits<char>, allocator<char> > ostringstream;
	typedef class _lyramilk_api_ std::basic_ostringstream<unsigned char, std::char_traits<unsigned char>, allocator<unsigned char> > odatastream;
	typedef class _lyramilk_api_ std::basic_ostringstream<wchar_t, std::char_traits<wchar_t>, allocator<wchar_t> > wostringstream;

	typedef class _lyramilk_api_ std::basic_iostream<char> stream;
	typedef class _lyramilk_api_ std::basic_iostream<unsigned char> bstream;
	typedef class _lyramilk_api_ std::basic_iostream<wchar_t> wstream;

	typedef class _lyramilk_api_ std::basic_istream<char> istream;
	typedef class _lyramilk_api_ std::basic_istream<unsigned char> bistream;
	typedef class _lyramilk_api_ std::basic_istream<wchar_t> wistream;

	typedef class _lyramilk_api_ std::basic_ostream<char> ostream;
	typedef class _lyramilk_api_ std::basic_ostream<unsigned char> bostream;
	typedef class _lyramilk_api_ std::basic_ostream<wchar_t> wostream;


	typedef class _lyramilk_api_ std::vector<lyramilk::data::string,allocator<lyramilk::data::string> > strings;


	class case_insensitive_hash{
	  public:
		size_t operator()(const lyramilk::data::string& a) const;
	};

	class case_insensitive_equare{
	  public:
		bool operator()(const lyramilk::data::string& a,const lyramilk::data::string& b) const;
	};

	class case_insensitive_less{
	  public:
		bool operator()(const lyramilk::data::string& a,const lyramilk::data::string& b) const;
	};

	typedef lyramilk::data::unordered_map<lyramilk::data::string,lyramilk::data::string,case_insensitive_hash ,case_insensitive_equare > case_insensitive_unordered_map;
	typedef std::map<lyramilk::data::string,lyramilk::data::string,case_insensitive_less> case_insensitive_map;


#ifdef USEMILKALLOC
	std::string inline str(const lyramilk::data::string& str)
	{
		return std::string(str.c_str(),str.size());
	}

	lyramilk::data::string inline str(const std::string& str)
	{
		return lyramilk::data::string(str.c_str(),str.size());
	}
#else
	std::string inline str(const lyramilk::data::string& str)
	{
		return str;
	}
#endif
	lyramilk::data::string inline str(unsigned long long i)
	{
		char buff[64];
		snprintf(buff,sizeof(buff),"%llu",i);
		return buff;
	}

	lyramilk::data::string inline str(long long i)
	{
		char buff[64];
		snprintf(buff,sizeof(buff),"%lld",i);
		return buff;
	}

	lyramilk::data::string inline str(unsigned long i)
	{
		char buff[64];
		snprintf(buff,sizeof(buff),"%lu",i);
		return buff;
	}

	lyramilk::data::string inline str(long i)
	{
		char buff[64];
		snprintf(buff,sizeof(buff),"%ld",i);
		return buff;
	}

	lyramilk::data::string inline str(unsigned int i)
	{
		char buff[64];
		snprintf(buff,sizeof(buff),"%u",i);
		return buff;
	}

	lyramilk::data::string inline str(int i)
	{
		char buff[64];
		snprintf(buff,sizeof(buff),"%d",i);
		return buff;
	}

	lyramilk::data::string inline str(double f)
	{
		char buff[128];
		snprintf(buff,sizeof(buff),"%f",f);
		return buff;
	}

	lyramilk::data::string inline str(float f)
	{
		char buff[128];
		snprintf(buff,sizeof(buff),"%f",f);
		return buff;
	}

#ifdef HAVE_UNORDEREDMAP
		typedef lyramilk::data::unordered_map<lyramilk::data::string,lyramilk::data::string> stringdict;
#else
		typedef std::map<lyramilk::data::string,lyramilk::data::string> stringdict;
#endif
#ifdef HAVE_UNORDEREDMAP
		typedef lyramilk::data::unordered_map<lyramilk::data::wstring,lyramilk::data::wstring> wstringdict;
#else
		typedef std::map<lyramilk::data::wstring,lyramilk::data::wstring> wstringdict;
#endif


	class _lyramilk_api_ type_invalid:public std::exception
	{
		lyramilk::data::string p;
	  public:
		type_invalid(const lyramilk::data::string& msg);
		virtual ~type_invalid() throw();
		virtual const char* what() const throw();
	};


}}

#ifdef Z_HAVE_UNORDEREDMAP
namespace std{
#elif defined Z_HAVE_TR1_UNORDEREDMAP
namespace std{namespace tr1{
#endif

#if (defined Z_HAVE_UNORDEREDMAP) || (defined Z_HAVE_TR1_UNORDEREDMAP)
	template <>
	size_t hash<const lyramilk::data::string&>::operator()(const lyramilk::data::string& ) const;
#endif
#ifdef Z_HAVE_UNORDEREDMAP
}
#elif defined Z_HAVE_TR1_UNORDEREDMAP
}}
#endif

_lyramilk_api_ lyramilk::data::bostream& operator << (lyramilk::data::bostream& os, char c);
_lyramilk_api_ lyramilk::data::bostream& operator << (lyramilk::data::bostream& os, signed char c);
_lyramilk_api_ lyramilk::data::bostream& operator << (lyramilk::data::bostream& os, const char* c);
_lyramilk_api_ lyramilk::data::bostream& operator << (lyramilk::data::bostream& os, const signed char* c);

#endif
