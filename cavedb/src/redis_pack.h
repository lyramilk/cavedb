#ifndef _cavedb_redis_pack_h_
#define _cavedb_redis_pack_h_
#include "slice.h"
#include <libmilk/def.h>
#include <leveldb/slice.h>

namespace lyramilk{ namespace cave
{

	struct type_error
	{};


	struct string_too_large
	{};

	struct redis_pack
	{
		const static char magic = 0xbb;
		const static char magiceof = 0xbc;
		enum stype{
			s_any		= 0x02,	//  msm
			s_string	= 0x10,	//	msmt
			s_hash		= 0x20,	//	msmtms
			s_list		= 0x30,	//	msmtmi
			s_set		= 0x40,	//	msmtms
			s_zset		= 0x50,	//	msmtmfms
			s_zset_m2s	= 0x51,	//	msmtms
			s_native	= 0x70,	//	msmt
			s_eof		= 0xff,	//	msmt
		}type;

		cavedb::Slice key;

		union{
			struct{
				cavedb::Slice2 field;
			}hash;
			struct{
				double score;
				cavedb::Slice2 member;
			}zset;
			struct{
				cavedb::Slice2 member;
			}set;
			struct{
				lyramilk::data::uint64 seq;
			}list;
		};

		redis_pack()
		{}

		~redis_pack()
		{}

		static bool unpack(redis_pack* s,leveldb::Slice key);
		static std::string pack(redis_pack* s);
		static std::string make_key_prefix(leveldb::Slice key);
		static std::string make_key_eof(leveldb::Slice key);
		static std::string make_hashmap_prefix(leveldb::Slice key,leveldb::Slice field);

		static int Compare(const leveldb::Slice& a, const leveldb::Slice& b);
		static void FindShortestSeparator(std::string* start,const leveldb::Slice& limit);
		static void FindShortSuccessor(std::string* key);
	};

	struct redis_pack2:public redis_pack
	{
		std::string data;
	};

}}
#endif