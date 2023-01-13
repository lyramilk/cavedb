#ifndef _cavedb_binlog_store_h_
#define _cavedb_binlog_store_h_

#include <libmilk/var.h>

namespace leveldb{class DB;};

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class binlog_reader
	{
	  public:
		virtual bool recv_callback(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args);
	};


	class binlog
	{
		leveldb::DB* ldb;
		lyramilk::data::uint64 seq;
		static void* thread_clear_binlog(binlog* blog);
	  public:
		binlog();
		virtual ~binlog();

		lyramilk::data::uint64 find_min();
		lyramilk::data::uint64 find_max();

		bool open_leveldb(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing);
		virtual bool append(const lyramilk::data::array& args);
		virtual void read(lyramilk::data::uint64 seq,binlog_reader* reader);
	};

}}
#endif
