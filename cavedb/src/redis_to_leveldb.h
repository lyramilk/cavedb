#ifndef _casedb_redis_to_leveldb_h_
#define _casedb_redis_to_leveldb_h_

#include <libmilk/var.h>
#include <libmilk/factory.h>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <leveldb/comparator.h>

namespace lyramilk{ namespace cave
{
	//	: ==> sep
	class redis_leveldb_comparator:public leveldb::Comparator
	{
	  public:
		redis_leveldb_comparator();
		virtual ~redis_leveldb_comparator();
		virtual int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const;
		virtual const char* Name() const;
		virtual void FindShortestSeparator(std::string* start,const leveldb::Slice& limit) const;
		virtual void FindShortSuccessor(std::string* key) const;
	};

	class redis_leveldb_handler
	{
		std::string dbid;
		lyramilk::data::uint64 idbid;
	  public:
		leveldb::DB* ldb;
		leveldb::ReadOptions ropt;
	  public:
		static lyramilk::data::int64 mstime();
		static std::string double2bytes(double s);
		static double bytes2double(const lyramilk::data::string& str);
		static double bytes2double(const leveldb::Slice& str);

		static std::string integer2bytes(lyramilk::data::uint64 s);
		static lyramilk::data::uint64 bytes2integer(const lyramilk::data::string& str);
		static lyramilk::data::uint64 bytes2integer(const leveldb::Slice& str);

		redis_leveldb_handler();
		~redis_leveldb_handler();

		std::string dbprefix(lyramilk::data::uint64 dbid);
		//	KEY
		//	键中，无法判断一个不定长字符串的结尾。所以最多有一个不定长字符串参数，由于key在所有情况下都是结定的，所以key并不是不定长字符串。
		//	d:[dbid]:b:[key]:type => type
		//std::string encode_type_key(const lyramilk::data::string& key,lyramilk::data::uint64 dbid = -1);
		//	d:[dbid]:b:[key]:ttl => ttl
		std::string encode_ttl_key(const lyramilk::data::string& key,lyramilk::data::uint64 dbid = -1);
		//	d:[dbid]:b:[key]:size => item count
		std::string encode_size_key(const lyramilk::data::string& key,lyramilk::data::uint64 dbid = -1);

		//	STRING
		//	d:[dbid]:s:[key]:v => value
		std::string encode_string_key(const lyramilk::data::string& key,lyramilk::data::uint64 dbid = -1);

		//	ZSET
		//	d:[dbid]:z:[key]:s:[score]:[value]	=> empty
		std::string encode_zset_score_key(const lyramilk::data::string& key,double score,const lyramilk::data::string& value,lyramilk::data::uint64 dbid = -1);
		//	d:[dbid]:z:[key]:v:[value]	=> score
		std::string encode_zset_data_key(const lyramilk::data::string& key,const lyramilk::data::string& value,lyramilk::data::uint64 dbid = -1);

		//	SET
		//	d:[dbid]:e:[key]:v:[value]	=> empty
		std::string encode_set_data_key(const lyramilk::data::string& key,const lyramilk::data::string& value,lyramilk::data::uint64 dbid = -1);

		//	HASHMAP
		//	d:[dbid]:h:[key]:f:[field]	=> value
		std::string encode_hashmap_data_key(const lyramilk::data::string& key,const lyramilk::data::string& field,lyramilk::data::uint64 dbid = -1);

		//	LIST
		//	d:[dbid]:l:[key]:q:[seq]	=> value
		std::string encode_list_data_key(const lyramilk::data::string& key,lyramilk::data::uint64 seq,lyramilk::data::uint64 dbid = -1);


		void init(leveldb::DB* pldb);
		void select(lyramilk::data::uint64 dbid);
		lyramilk::data::uint64 get_dbid();

		lyramilk::data::uint64 get_size(const lyramilk::data::string& key,lyramilk::data::uint64 dbid = -1);
		bool incr_size(leveldb::WriteBatch &batch,const lyramilk::data::string& key);
		bool decr_size(leveldb::WriteBatch &batch,const lyramilk::data::string& key);
		void set_size(leveldb::WriteBatch &batch,const lyramilk::data::string& key,lyramilk::data::uint64 size);

		lyramilk::data::int64 get_pttl(const lyramilk::data::string& key,lyramilk::data::uint64 dbid = -1);
		void set_pttl(leveldb::WriteBatch &batch,const lyramilk::data::string& key,lyramilk::data::int64 expiretime);

		// list
		// index = 0 第一个元素之前，-1 最后一个元素之后
		lyramilk::data::uint64 get_list_seq(const lyramilk::data::string& key,lyramilk::data::int64 index,lyramilk::data::uint64 dbid = -1);

		void* userdata;
	};


	class redis_leveldb_key
	{
		std::string str;
	  public:
		static const unsigned char KEY_STR = 0x01;
		static const unsigned char KEY_RINT = 0x02;	//反整数，数值越大，键越小
		static const unsigned char KEY_INT = 0x03;
		static const unsigned char KEY_DOUBLE = 0x04;
		static const unsigned char KEY_NOOP = 0x05;
		static const unsigned char KEY_EOF = 0xff;

		static const unsigned char KT_BASE = 'b';
		static const unsigned char KT_STRING = 't';
		static const unsigned char KT_ZSET = 'z';
		static const unsigned char KT_SET = 's';
		static const unsigned char KT_LIST = 'l';
		static const unsigned char KT_HASH = 'h';
		//static const unsigned char KT_ALL[] = {redis_leveldb_key::KT_BASE,redis_leveldb_key::KT_STRING,redis_leveldb_key::KT_ZSET,redis_leveldb_key::KT_SET,redis_leveldb_key::KT_LIST,redis_leveldb_key::KT_HASH};

		redis_leveldb_key();
		redis_leveldb_key(const std::string& prefix);
		~redis_leveldb_key();
		void append_string(const char* str,std::size_t sz);
		void append_int(lyramilk::data::uint64 d);
		void append_rint(lyramilk::data::uint64 d);
		void append_double(double d);
		void append_type(unsigned char kt);
		void append_noop();
		void append_max();

		operator std::string();
		operator leveldb::Slice();
	};

	typedef void (redis_command_handle)(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args);

	void cavedb_noop(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args);

	class cavedb_redis_commands:public lyramilk::util::multiton_factory<redis_command_handle>
	{
		cavedb_redis_commands();
	  public:
		virtual ~cavedb_redis_commands();
		static cavedb_redis_commands* instance();
	};

}}

#endif
