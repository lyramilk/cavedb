#include "leveldb_minimal_adapter.h"
#include "leveldb_minimal.h"
#include "leveldb_minimal2.h"
#include <libmilk/log.h>
#include <libmilk/dict.h>

namespace lyramilk{ namespace cave
{
	lyramilk::log::logss static log(lyramilk::klog,"lyramilk.cave.store.leveldb_minimal_adapter");

	bool minimal_interface::notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
	{
		return true;
	}

	bool leveldb_minimal_adapter::notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
	{
		return adapter->notify_idle(replid,offset);
	}

	bool leveldb_minimal_adapter::notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
	{
		return adapter->notify_idle(replid,offset);
	}

	bool leveldb_minimal_adapter::notify_flushdb(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return adapter->notify_flushdb(replid,offset,args);
	}

	bool leveldb_minimal_adapter::notify_flushall(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return adapter->notify_flushall(replid,offset,args);
	}

	bool leveldb_minimal_adapter::notify_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return adapter->notify_del(replid,offset,args);
	}

	bool leveldb_minimal_adapter::notify_move(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return adapter->notify_move(replid,offset,args);
	}

	bool leveldb_minimal_adapter::notify_pexpireat(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return adapter->notify_pexpireat(replid,offset,args);
	}

	bool leveldb_minimal_adapter::notify_persist(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return adapter->notify_persist(replid,offset,args);
	}

	bool leveldb_minimal_adapter::notify_rename(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return adapter->notify_rename(replid,offset,args);
	}

	bool leveldb_minimal_adapter::notify_hset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return adapter->notify_hset(replid,offset,args);
	}

	bool leveldb_minimal_adapter::notify_hdel(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return adapter->notify_hdel(replid,offset,args);
	}


	bool leveldb_minimal_adapter::notify_set(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return adapter->notify_set(replid,offset,args);
	}

	bool leveldb_minimal_adapter::notify_ssdb_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return adapter->notify_ssdb_del(replid,offset,args);
	}

	bool leveldb_minimal_adapter::notify_ssdb_qset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return adapter->notify_ssdb_qset(replid,offset,args);
	}

	bool leveldb_minimal_adapter::notify_lpop(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return adapter->notify_lpop(replid,offset,args);
	}

	bool leveldb_minimal_adapter::notify_rpop(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return adapter->notify_rpop(replid,offset,args);
	}

	bool leveldb_minimal_adapter::notify_zadd(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return adapter->notify_zadd(replid,offset,args);
	}

	bool leveldb_minimal_adapter::notify_zrem(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return adapter->notify_zrem(replid,offset,args);
	}

	leveldb_minimal_adapter::leveldb_minimal_adapter()
	{
		version_map[leveldb_minimal::cfver].open_or_create_instance = leveldb_minimal::open;
		version_map[leveldb_minimal::cfver].format_instance = leveldb_minimal::open_focus;
		version_map[leveldb_minimal2::cfver].open_or_create_instance = leveldb_minimal2::open;
		version_map[leveldb_minimal2::cfver].format_instance = leveldb_minimal2::open_focus;

		default_version = leveldb_minimal2::cfver;
		adapter = nullptr;
	}

	leveldb_minimal_adapter::~leveldb_minimal_adapter()
	{
	}

	bool leveldb_minimal_adapter::open(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB)
	{
		leveldb_minimal_version_master::const_iterator it = version_map.begin();
		for(;it!=version_map.end() && adapter == nullptr;++it){
			if((void*)it->second.open_or_create_instance){
				adapter = it->second.open_or_create_instance(leveldbpath,cache_size_MB);
			}
		}
		if(adapter == nullptr && !version_map.empty()){
			it = version_map.find(default_version);
			if (it != version_map.end())
			{
				adapter = it->second.format_instance(leveldbpath,cache_size_MB);
			}
		}
		return adapter != nullptr;
	}

	long long leveldb_minimal_adapter::get_sigval()
	{
		if(adapter) return adapter->get_sigval();
		return 0;
	}

	bool leveldb_minimal_adapter::compact()
	{
		if(adapter) return adapter->compact();
		return false;
	}

	lyramilk::data::string leveldb_minimal_adapter::get_leveldb_property(const lyramilk::data::string& property)
	{
		if(adapter) return adapter->get_leveldb_property(property);
		return "";
	}

	bool leveldb_minimal_adapter::get_sync_info(lyramilk::data::string* replid,lyramilk::data::uint64* offset) const
	{
		if(adapter) return adapter->get_sync_info(replid,offset);
		return false;
	}

	bool leveldb_minimal_adapter::hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		if(adapter) return adapter->hexist(key,field);
		return false;
	}

	lyramilk::data::string leveldb_minimal_adapter::hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		if(adapter) return adapter->hget(key,field);
		return false;
	}

	lyramilk::data::stringdict leveldb_minimal_adapter::hgetall(const lyramilk::data::string& key) const
	{
		if(adapter) return adapter->hgetall(key);
		return lyramilk::data::stringdict();
	}
}}
