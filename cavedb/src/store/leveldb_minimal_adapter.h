#ifndef _cavedb_leveldb_minimal_adapter_h_
#define _cavedb_leveldb_minimal_adapter_h_

#include <libmilk/var.h>
#include <libmilk/thread.h>
#include "../store.h"
#include "../store_reader.h"
#include "../redis_like_session.h"

/*
	由于打算支持多种容器， 1_mininal 的键构造对多种容器时leveldb缓存不友好，故增加 2_mininal 版本，这一版本对leveldb排键时会把同种容器排在一起。
*/

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{

	class minimal_interface:public lyramilk::cave::store,public lyramilk::cave::store_reader
	{
		friend class leveldb_minimal_adapter;
	  protected:
		virtual bool notify_idle(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata);
	  public:
		virtual bool compact() = 0;
		virtual lyramilk::data::string get_property(const lyramilk::data::string& property) = 0;

		virtual bool get_sync_info(const lyramilk::data::string& masterid,lyramilk::data::string* replid,lyramilk::data::uint64* offset) const = 0;
		virtual bool hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const = 0;
		virtual lyramilk::data::string hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const = 0;
		virtual lyramilk::data::stringdict hgetall(const lyramilk::data::string& key) const = 0;
	};



	struct minimal_version
	{
		minimal_interface*(*open_instance)(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing);
	};


	class leveldb_minimal_adapter:public minimal_interface
	{
	  protected:
		minimal_interface* adapter;
	  protected:
		typedef std::map<lyramilk::data::string,minimal_version> leveldb_minimal_version_master;
		leveldb_minimal_version_master leveldb_version_map;
		lyramilk::data::string leveldb_default_version;
	  protected:
		lyramilk::data::string ver;
	  protected:
		virtual bool notify_idle(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata);
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

	  public:
		leveldb_minimal_adapter();
		virtual ~leveldb_minimal_adapter();
		bool open_leveldb(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing = false);
		bool open(minimal_interface* adapter);
	  public:
		virtual bool compact();
		virtual lyramilk::data::string get_property(const lyramilk::data::string& property);
		virtual bool is_on_full_sync();
	  public:
		virtual lyramilk::data::uint64 rspeed() const;
		virtual bool get_sync_info(const lyramilk::data::string& masterid,lyramilk::data::string* replid,lyramilk::data::uint64* offset) const;
		virtual bool hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::string hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::stringdict hgetall(const lyramilk::data::string& key) const;
	};

	class leveldb_minimal_redislike_session : public redislike_session
	{
	  	bool session_with_monitor;
		lyramilk::data::string subscribe_channel;
		lyramilk::cave::leveldb_minimal_adapter* dbins;
		static redislike_dispatch_type dispatch;
	  public:
		leveldb_minimal_redislike_session();
		virtual ~leveldb_minimal_redislike_session();
		static void static_init_dispatch();
		virtual void init_cavedb(const lyramilk::data::string& masterid,const lyramilk::data::string& requirepass,lyramilk::cave::leveldb_minimal_adapter* dbins,bool readonly);
		virtual result_status notify_info(const lyramilk::data::array& cmd, std::ostream& os);
	};

}}

#endif
