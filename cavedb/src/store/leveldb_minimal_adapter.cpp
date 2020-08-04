#include "leveldb_minimal_adapter.h"
#include "leveldb_minimal.h"
#include "leveldb_minimal2.h"
#include "rocksdb_minimal.h"
#include <libmilk/log.h>
#include <libmilk/dict.h>

namespace lyramilk{ namespace cave
{
	lyramilk::log::logss static log(lyramilk::klog,"lyramilk.cave.store.leveldb_minimal_adapter");

	bool minimal_interface::notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata)
	{
		return true;
	}

	bool leveldb_minimal_adapter::notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata)
	{
		return adapter->notify_idle(replid,offset,userdata);
	}

	bool leveldb_minimal_adapter::notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata)
	{
		return adapter->notify_idle(replid,offset,userdata);
	}

	bool leveldb_minimal_adapter::notify_flushdb(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return adapter->notify_flushdb(replid,offset,args,userdata);
	}

	bool leveldb_minimal_adapter::notify_flushall(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return adapter->notify_flushall(replid,offset,args,userdata);
	}

	bool leveldb_minimal_adapter::notify_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return adapter->notify_del(replid,offset,args,userdata);
	}

	bool leveldb_minimal_adapter::notify_move(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return adapter->notify_move(replid,offset,args,userdata);
	}

	bool leveldb_minimal_adapter::notify_pexpireat(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return adapter->notify_pexpireat(replid,offset,args,userdata);
	}

	bool leveldb_minimal_adapter::notify_persist(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return adapter->notify_persist(replid,offset,args,userdata);
	}

	bool leveldb_minimal_adapter::notify_rename(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return adapter->notify_rename(replid,offset,args,userdata);
	}

	bool leveldb_minimal_adapter::notify_hset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return adapter->notify_hset(replid,offset,args,userdata);
	}

	bool leveldb_minimal_adapter::notify_hdel(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return adapter->notify_hdel(replid,offset,args,userdata);
	}


	bool leveldb_minimal_adapter::notify_set(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return adapter->notify_set(replid,offset,args,userdata);
	}

	bool leveldb_minimal_adapter::notify_ssdb_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return adapter->notify_ssdb_del(replid,offset,args,userdata);
	}

	bool leveldb_minimal_adapter::notify_ssdb_qset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return adapter->notify_ssdb_qset(replid,offset,args,userdata);
	}

	bool leveldb_minimal_adapter::notify_lpop(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return adapter->notify_lpop(replid,offset,args,userdata);
	}

	bool leveldb_minimal_adapter::notify_rpop(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return adapter->notify_rpop(replid,offset,args,userdata);
	}

	bool leveldb_minimal_adapter::notify_zadd(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return adapter->notify_zadd(replid,offset,args,userdata);
	}

	bool leveldb_minimal_adapter::notify_zrem(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return adapter->notify_zrem(replid,offset,args,userdata);
	}

	leveldb_minimal_adapter::leveldb_minimal_adapter()
	{
		leveldb_version_map[leveldb_minimal::cfver].open_instance = leveldb_minimal::open;
		leveldb_version_map[leveldb_minimal2::cfver].open_instance = leveldb_minimal2::open;

		leveldb_default_version = leveldb_minimal2::cfver;
		adapter = nullptr;
	}

	leveldb_minimal_adapter::~leveldb_minimal_adapter()
	{
	}

	bool leveldb_minimal_adapter::open_leveldb(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing)
	{
		leveldb_minimal_version_master::const_iterator it = leveldb_version_map.begin();
		for(;it!=leveldb_version_map.end() && adapter == nullptr;++it){
			if((void*)it->second.open_instance){
				adapter = it->second.open_instance(leveldbpath,cache_size_MB,false);
			}
		}

		if(create_if_missing){
			if(adapter == nullptr){
				adapter = leveldb_version_map[leveldb_default_version].open_instance(leveldbpath,cache_size_MB,true);
			}
		}
		return adapter != nullptr;
	}

	bool leveldb_minimal_adapter::open(minimal_interface* adapter)
	{
		this->adapter = adapter;
		return adapter != nullptr;
	}

	bool leveldb_minimal_adapter::compact()
	{
		if(adapter) return adapter->compact();
		return false;
	}

	lyramilk::data::string leveldb_minimal_adapter::get_property(const lyramilk::data::string& property)
	{
		if(adapter) return adapter->get_property(property);
		return "";
	}

	bool leveldb_minimal_adapter::is_on_full_sync()
	{
		return on_full_sync();
	}

	lyramilk::data::uint64 leveldb_minimal_adapter::rspeed() const
	{
		if(adapter) return adapter->rspeed();
		return 0;
	}

	bool leveldb_minimal_adapter::get_sync_info(lyramilk::data::string* replid,lyramilk::data::uint64* offset) const
	{
		if(adapter) return adapter->get_sync_info(replid,offset);
		return false;
	}

	bool leveldb_minimal_adapter::hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		if(adapter){
			return adapter->hexist(key,field);
		}
		return false;
	}

	lyramilk::data::string leveldb_minimal_adapter::hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		if(adapter){
			return adapter->hget(key,field);
		}
		return "";
	}

	lyramilk::data::stringdict leveldb_minimal_adapter::hgetall(const lyramilk::data::string& key) const
	{
		if(adapter){
			return adapter->hgetall(key);
		}
		return lyramilk::data::stringdict();
	}
}}




