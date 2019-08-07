#ifndef _lyramilk_cryptology_hash_h_
#define _lyramilk_cryptology_hash_h_

#include <string>

namespace lyramilk{ namespace cryptology
{
	typedef unsigned int (*hash32_type)(const char* p,std::size_t l);
	typedef unsigned long long (*hash64_type)(const char* p,std::size_t l);

	namespace hash32{
		unsigned int bkdr(const char* p,std::size_t l);
		unsigned int djb(const char* p,std::size_t l);
		unsigned int ap(const char* p,std::size_t l);
		unsigned int dek(const char* p,std::size_t l);
		unsigned int murmur2(const char* p,std::size_t l);
		unsigned int fnv(const char* p,std::size_t l);
	}

	namespace hash64{
		unsigned long long murmur2(const char* p,std::size_t l);
		unsigned long long fnv(const char* p,std::size_t l);
		unsigned long long fnvX(const char* p,std::size_t l);
	}
}}
#endif