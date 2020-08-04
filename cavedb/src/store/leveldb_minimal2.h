#ifndef _cavedb_leveldb_minimal2_h_
#define _cavedb_leveldb_minimal2_h_

#include <libmilk/var.h>
#include <libmilk/thread.h>
#include "../store.h"
#include "../store_reader.h"
#include "leveldb_minimal_adapter.h"

namespace leveldb{class DB;};

/*
	由于打算支持多种容器， 1_mininal 的键构造对多种容器时leveldb缓存不友好，故增加 2_mininal 版本，这一版本对leveldb排键时会把同种容器排在一起。
*/

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class leveldb_minimal2:public minimal_interface
	{
	  protected:
		leveldb::DB* ldb;
	  protected:
		virtual bool notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata);
		virtual bool notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata);
		// db
		virtual bool notify_flushdb(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_flushall(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		// key
		virtual bool notify_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_move(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_pexpireat(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_persist(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_rename(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		// hashmap
		virtual bool notify_hset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_hdel(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		// kv
		virtual bool notify_set(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_ssdb_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		// queue
		virtual bool notify_ssdb_qset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_lpop(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_rpop(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		// zset
		virtual bool notify_zadd(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_zrem(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);

		leveldb_minimal2();
	  public:
		const static std::string cfver;
		virtual ~leveldb_minimal2();
		static minimal_interface* open(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing);
		bool compact();
	  public:
		//	leveldb.num-files-at-level<N>
		//	leveldb.stats
		//	leveldb.sstables
		virtual lyramilk::data::string get_property(const lyramilk::data::string& property);

		virtual bool get_sync_info(lyramilk::data::string* replid,lyramilk::data::uint64* offset) const;
		virtual bool hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::string hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual bool hget(const lyramilk::data::string& key,const lyramilk::data::string& field,lyramilk::data::string* value) const;
		virtual lyramilk::data::stringdict hgetall(const lyramilk::data::string& key) const;
	};
}}

#endif
