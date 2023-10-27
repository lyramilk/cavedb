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
		virtual lyramilk::data::uint64 find_min() = 0;
		virtual lyramilk::data::uint64 find_max() = 0;
		virtual bool append(const lyramilk::data::array& args) = 0;
		virtual void read(lyramilk::data::uint64 seq,lyramilk::data::uint64 count,lyramilk::data::array* data,lyramilk::data::uint64* nextseq) = 0;
	};


	class binlog_leveldb:public binlog
	{
		leveldb::DB* ldb;
		lyramilk::data::uint64 seq;
		lyramilk::data::uint64 minseq;
		lyramilk::data::uint64 maxseq;
		lyramilk::data::uint64 capacity;
		static void* thread_clear_binlog(binlog_leveldb* blog);
	  public:
		binlog_leveldb();
		virtual ~binlog_leveldb();

		lyramilk::data::uint64 find_min();
		lyramilk::data::uint64 find_max();

		bool open_leveldb(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing,long capacity);
		virtual bool append(const lyramilk::data::array& args);
		virtual void read(lyramilk::data::uint64 seq,lyramilk::data::uint64 count,lyramilk::data::array* data,lyramilk::data::uint64* nextseq);
	};



}}
#endif
