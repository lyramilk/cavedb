#ifndef _cavedb_binlog_store_h_
#define _cavedb_binlog_store_h_

#include <libmilk/var.h>

namespace leveldb{class DB;};

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class binlog
	{
	  public:
		virtual void set_master(bool is_master) = 0;

		virtual lyramilk::data::uint64 find_min() = 0;
		virtual lyramilk::data::uint64 find_max() = 0;

		virtual bool hset(const lyramilk::data::string& key,const lyramilk::data::string& field,const lyramilk::data::string& value) = 0;
		virtual bool hdel(const lyramilk::data::string& key,const lyramilk::data::string& field) = 0;

		virtual bool sadd(const lyramilk::data::string& key,const lyramilk::data::string& member) = 0;
		virtual bool srem(const lyramilk::data::string& key,const lyramilk::data::string& member) = 0;

		virtual bool zadd(const lyramilk::data::string& key,double score,const lyramilk::data::string& value) = 0;
		virtual bool zrem(const lyramilk::data::string& key,const lyramilk::data::string& value) = 0;


		virtual bool hset_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,const lyramilk::data::string& field,const lyramilk::data::string& value) = 0;
		virtual bool hdel_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,const lyramilk::data::string& field) = 0;

		virtual bool sadd_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,const lyramilk::data::string& member) = 0;
		virtual bool srem_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,const lyramilk::data::string& member) = 0;

		virtual bool zadd_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,double score,const lyramilk::data::string& value) = 0;
		virtual bool zrem_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,const lyramilk::data::string& value) = 0;

		virtual bool read(lyramilk::data::uint64 seq,lyramilk::data::uint64 count,lyramilk::data::array* data,lyramilk::data::uint64* nextseq,bool withseq) = 0;
	};


	class binlog_leveldb:public binlog
	{
		leveldb::DB* ldb;
		lyramilk::data::uint64 seq;
		lyramilk::data::uint64 minseq;
		lyramilk::data::uint64 maxseq;
		lyramilk::data::uint64 capacity;
		bool is_master;
		static void* thread_clear_binlog(binlog_leveldb* blog);
	  public:
		binlog_leveldb();
		virtual ~binlog_leveldb();
		
		void set_master(bool is_master);

		lyramilk::data::uint64 find_min();
		lyramilk::data::uint64 find_max();

		bool open_leveldb(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing,long capacity);
		//virtual bool append(const lyramilk::data::array& args);


		virtual bool hset(const lyramilk::data::string& key,const lyramilk::data::string& field,const lyramilk::data::string& value);
		virtual bool hdel(const lyramilk::data::string& key,const lyramilk::data::string& field);

		virtual bool sadd(const lyramilk::data::string& key,const lyramilk::data::string& member);
		virtual bool srem(const lyramilk::data::string& key,const lyramilk::data::string& member);

		virtual bool zadd(const lyramilk::data::string& key,double score,const lyramilk::data::string& value);
		virtual bool zrem(const lyramilk::data::string& key,const lyramilk::data::string& value);

		virtual bool hset_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,const lyramilk::data::string& field,const lyramilk::data::string& value);
		virtual bool hdel_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,const lyramilk::data::string& field);

		virtual bool sadd_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,const lyramilk::data::string& member);
		virtual bool srem_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,const lyramilk::data::string& member);

		virtual bool zadd_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,double score,const lyramilk::data::string& value);
		virtual bool zrem_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,const lyramilk::data::string& value);

		virtual bool read(lyramilk::data::uint64 seq,lyramilk::data::uint64 count,lyramilk::data::array* data,lyramilk::data::uint64* nextseq,bool withseq);
	};



}}
#endif
