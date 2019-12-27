#include "def.h"
#include <algorithm>

#ifdef Z_HAVE_JEMALLOC
	#include <jemalloc/jemalloc.h>
#else
	#include <malloc.h>
#endif

std::locale::id id;

namespace lyramilk { namespace data{
	void* milk_malloc(size_t size)
	{
		return ::malloc(size);
	}

	void milk_free(void* p, size_t size)
	{
		::free(p);
	}

	type_invalid::type_invalid(const string& msg)
	{
		p = msg;
	}

	type_invalid::~type_invalid() throw()
	{
	}

	const char* type_invalid::what() const throw()
	{
		return p.c_str();
	}


	size_t case_insensitive_hash::operator()(const lyramilk::data::string& a) const
	{
		lyramilk::data::string sa(a.size(),0);
		transform(a.begin(), a.end(), sa.begin(), tolower);
		return lyramilk::data::hash<const lyramilk::data::string& >()(sa);
	}

	bool case_insensitive_equare::operator()(const lyramilk::data::string& a,const lyramilk::data::string& b) const
	{
		lyramilk::data::string sa(a.size(),0);
		transform(a.begin(), a.end(), sa.begin(), tolower);

		lyramilk::data::string sb(b.size(),0);
		transform(b.begin(), b.end(), sb.begin(), tolower);

		return sa == sb;
	}

	bool case_insensitive_less::operator()(const lyramilk::data::string& a,const lyramilk::data::string& b) const
	{
		lyramilk::data::string sa(a.size(),0);
		transform(a.begin(), a.end(), sa.begin(), tolower);

		lyramilk::data::string sb(b.size(),0);
		transform(b.begin(), b.end(), sb.begin(), tolower);
		return sa < sb;
	}
}}	//namespace

lyramilk::data::bostream& operator << (lyramilk::data::bostream& os, char c)
{
	return os << (unsigned char)c;
}

lyramilk::data::bostream& operator << (lyramilk::data::bostream& os, signed char c)
{
	return os << (unsigned char)c;
}

lyramilk::data::bostream& operator << (lyramilk::data::bostream& os, const char* c)
{
	return os << (const unsigned char*)c;
}

lyramilk::data::bostream& operator << (lyramilk::data::bostream& os, const signed char* c)
{
	return os << (const unsigned char*)c;
}



#ifdef Z_HAVE_UNORDEREDMAP
namespace std{
#elif defined Z_HAVE_TR1_UNORDEREDMAP
namespace std{namespace tr1{
#endif

#if (defined Z_HAVE_UNORDEREDMAP) || (defined Z_HAVE_TR1_UNORDEREDMAP)
	template <>
	std::size_t hash<const lyramilk::data::string&>::operator()(const lyramilk::data::string& d) const
	{
		const static std::size_t seed = 0xee6b27eb;
		const static std::size_t m = 0xc6a4a7935bd1e995ULL;
		const static int r = 47;

		const std::size_t* p = (const std::size_t*)d.c_str();
		std::size_t l = d.size();

		const std::size_t* end = p + (l/8);
		std::size_t h = seed ^ (l * m);

		while(p != end)
		{
			std::size_t k = *p++;

			k *= m; 
			k ^= k >> r; 
			k *= m; 

			h ^= k;
			h *= m; 
		}

		const unsigned char * p2 = (const unsigned char*)p;

		switch(l & 7)
		{
		  case 7: h ^= std::size_t(p2[6]) << 48;
		  case 6: h ^= std::size_t(p2[5]) << 40;
		  case 5: h ^= std::size_t(p2[4]) << 32;
		  case 4: h ^= std::size_t(p2[3]) << 24;
		  case 3: h ^= std::size_t(p2[2]) << 16;
		  case 2: h ^= std::size_t(p2[1]) << 8;
		  case 1: h ^= std::size_t(p2[0]);
			h *= m;
		};

		h ^= h >> r;
		h *= m;
		h ^= h >> r;

		return h;
	}
#endif
#ifdef Z_HAVE_UNORDEREDMAP
}
#elif defined Z_HAVE_TR1_UNORDEREDMAP
}}
#endif
