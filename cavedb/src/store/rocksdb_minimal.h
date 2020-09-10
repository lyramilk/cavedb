#ifndef _cavedb_rocksdb_minimal_h_
#define _cavedb_rocksdb_minimal_h_

#include <libmilk/var.h>
#include <libmilk/thread.h>
#include "../store.h"
#include "../store_reader.h"
#include "leveldb_minimal_adapter.h"

namespace rocksdb{class DB;};

/*
	由于打算支持多种容器， 1_mininal 的键构造对多种容器时rocksdb缓存不友好，故增加 2_mininal 版本，这一版本对rocksdb排键时会把同种容器排在一起。
*/

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class rocksdb_minimal:public lyramilk::cave::store,public lyramilk::cave::store_reader
	{
		rocksdb::DB* ldb;
	  protected:
		virtual bool notify_idle(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset);
		virtual bool notify_psync(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata);
		// db
		virtual bool notify_flushdb(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_flushall(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		// key
		virtual bool notify_del(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_move(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_pexpireat(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_persist(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_rename(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		// hashmap
		virtual bool notify_hset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_hdel(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		// kv
		virtual bool notify_set(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_ssdb_del(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		// queue
		virtual bool notify_ssdb_qset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_lpop(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_rpop(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		// zset
		virtual bool notify_zadd(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_zrem(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);

		rocksdb_minimal();
	  public:
		const static std::string cfver;
		virtual ~rocksdb_minimal();
		static rocksdb_minimal* open(const lyramilk::data::string& rocksdbpath,unsigned int cache_size_MB,bool create_if_missing);
		bool compact();
	  public:
		//	rocksdb.num-files-at-level<N>
		//	rocksdb.stats
		//	rocksdb.sstables
		virtual lyramilk::data::string get_leveldb_property(const lyramilk::data::string& property);

		virtual bool get_sync_info(const lyramilk::data::string& masterid,lyramilk::data::string* replid,lyramilk::data::uint64* offset) const;
		virtual bool hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::string hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::stringdict hgetall(const lyramilk::data::string& key) const;
	};
}}

#endif
