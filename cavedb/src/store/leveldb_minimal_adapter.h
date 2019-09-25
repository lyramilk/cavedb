#ifndef _cavedb_leveldb_minimal_adapter_h_
#define _cavedb_leveldb_minimal_adapter_h_

#include <libmilk/var.h>
#include <libmilk/thread.h>
#include "../store.h"
#include "../store_reader.h"

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
		virtual bool notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset);
	  public:
		virtual long long get_sigval() = 0;
		virtual bool compact() = 0;
		virtual lyramilk::data::string get_leveldb_property(const lyramilk::data::string& property) = 0;

		virtual bool get_sync_info(lyramilk::data::string* replid,lyramilk::data::uint64* offset) const = 0;
		virtual bool hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const = 0;
		virtual lyramilk::data::string hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const = 0;
		virtual lyramilk::data::stringdict hgetall(const lyramilk::data::string& key) const = 0;
	};



	struct minimal_version
	{
		minimal_interface*(*open_or_create_instance)(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB);
		minimal_interface*(*format_instance)(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB);
	};


	class leveldb_minimal_adapter:public minimal_interface
	{
		minimal_interface* adapter;
	  protected:
		typedef std::map<lyramilk::data::string,minimal_version> leveldb_minimal_version_master;
		leveldb_minimal_version_master version_map;
		lyramilk::data::string default_version;
	  protected:
		lyramilk::data::string ver;
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
		leveldb_minimal_adapter();
		virtual ~leveldb_minimal_adapter();
		bool open(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB);
		bool open_focus(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB);
	  public:
		virtual long long get_sigval();
		virtual bool compact();
		virtual lyramilk::data::string get_leveldb_property(const lyramilk::data::string& property);
		virtual bool is_on_full_sync();
	  public:
		virtual lyramilk::data::uint64 rspeed() const;
		virtual bool get_sync_info(lyramilk::data::string* replid,lyramilk::data::uint64* offset) const;
		virtual bool hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::string hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::stringdict hgetall(const lyramilk::data::string& key) const;
	};

}}

#endif
