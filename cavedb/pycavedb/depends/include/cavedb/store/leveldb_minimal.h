#ifndef _casedb_leveldb_minimal_h_
#define _casedb_leveldb_minimal_h_

#include <libmilk/var.h>
#include <libmilk/thread.h>
#include "../store.h"
#include "../store_reader.h"

namespace leveldb{class DB;};

/*
	由于打算支持多种容器， 1_mininal 的键构造对多种容器时leveldb缓存不友好，故增加 2_mininal 版本，这一版本对leveldb排键时会把同种容器排在一起。
*/

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{

	class leveldb_minimal:public lyramilk::cave::store,public lyramilk::cave::store_reader
	{
		leveldb::DB* ldb;
		int ver;
	  protected:
		mutable lyramilk::threading::mutex_semaphore sem;
	  protected:
		virtual bool notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset);
		virtual bool notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset);
		// db
		virtual bool notify_flushdb(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		virtual bool notify_flushall(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		// key
		virtual bool notify_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		virtual bool notify_move(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		virtual bool notify_pexpireat(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		virtual bool notify_persist(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		virtual bool notify_rename(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		// hashmap
		virtual bool notify_hset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		virtual bool notify_hdel(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		// kv
		virtual bool notify_set(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		virtual bool notify_ssdb_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		// queue
		virtual bool notify_ssdb_qset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		virtual bool notify_lpop(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		virtual bool notify_rpop(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		// zset
		virtual bool notify_zadd(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		virtual bool notify_zrem(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);

	  public:
		const static std::string cfver;
		leveldb_minimal();
		virtual ~leveldb_minimal();
		bool open(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB);
		bool compact();

		long long get_sigval();
	  public:
		//	leveldb.num-files-at-level<N>
		//	leveldb.stats
		//	leveldb.sstables
		virtual lyramilk::data::string get_leveldb_property(const lyramilk::data::string& property);

		virtual bool get_sync_info(lyramilk::data::string* replid,lyramilk::data::uint64* offset) const;
		virtual bool hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::string hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::stringdict hgetall(const lyramilk::data::string& key) const;
	};
}}

#endif
