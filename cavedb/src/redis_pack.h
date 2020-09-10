#ifndef _cavedb_redis_pack_h_
#define _cavedb_redis_pack_h_
#include "slice.h"
#include <libmilk/def.h>

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
		}type;

		cavedb::Slice key;

		union{
			struct{
				cavedb::Slice field;
			}hash;
			struct{
				double score;
				cavedb::Slice member;
			}zset;
			struct{
				cavedb::Slice member;
			}set;
			struct{
				lyramilk::data::uint64 seq;
			}list;
		};

		redis_pack()
		{}

		~redis_pack()
		{}

		static bool unpack(redis_pack* s,cavedb::Slice key);
		static std::string pack(redis_pack* s);
		static std::string make_key_prefix(cavedb::Slice key);
		static std::string make_hashmap_prefix(cavedb::Slice key,cavedb::Slice field);

		static int Compare(const cavedb::Slice& a, const cavedb::Slice& b);
		static void FindShortestSeparator(std::string* start,const cavedb::Slice& limit);
		static void FindShortSuccessor(std::string* key);
	};

	struct redis_pack2:public redis_pack
	{
		std::string data;
	};

}}
#endif