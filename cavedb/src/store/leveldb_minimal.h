#ifndef _casedb_leveldb_minimal_h_
#define _casedb_leveldb_minimal_h_

#include <libmilk/var.h>
#include <libmilk/thread.h>
#include "../store.h"
#include "../store_reader.h"

namespace leveldb{class DB;};

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class leveldb_minimal:public lyramilk::cave::store,public lyramilk::cave::store_reader
	{
		leveldb::DB* ldb;
	  protected:
		mutable lyramilk::threading::mutex_semaphore sem;
	  protected:
		virtual bool notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset);
		virtual bool notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset);
		// db
		virtual void notify_flushdb(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_flushall(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		// key
		virtual void notify_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_move(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_pexpireat(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_persist(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_rename(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		// hashmap
		virtual void notify_hset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_hdel(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);

	  public:
		const static std::string cfver;
		leveldb_minimal();
		virtual ~leveldb_minimal();
		bool open(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB);
		bool compact();

		long long get_sigval();
	  public:
		virtual bool get_sync_info(lyramilk::data::string* replid,lyramilk::data::uint64* offset) const;
		virtual bool hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::string hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::stringdict hgetall(const lyramilk::data::string& key) const;
	};
}}

#endif
