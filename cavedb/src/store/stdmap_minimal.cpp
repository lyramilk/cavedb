#include "stdmap_minimal.h"
#include "slave_redis.h"
#include "slave_ssdb.h"


namespace lyramilk{ namespace cave
{
	bool stdmap_minimal::notify_psync(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata)
	{
		return true;
	}

	bool stdmap_minimal::notify_flushdb(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		lyramilk::threading::mutex_sync _(lock.w());
		data.clear();
		return true;
	}

	bool stdmap_minimal::notify_flushall(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		lyramilk::threading::mutex_sync _(lock.w());
		data.clear();
		return true;
	}

	bool stdmap_minimal::notify_del(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		lyramilk::threading::mutex_sync _(lock.w());
		data.erase(args[1].str());
		return true;
	}

	bool stdmap_minimal::notify_move(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return false;
	}

	bool stdmap_minimal::notify_pexpireat(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return false;
	}

	bool stdmap_minimal::notify_persist(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return false;
	}

	bool stdmap_minimal::notify_rename(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		lyramilk::threading::mutex_sync _(lock.w());
		data[args[2]] = data[args[1]];
		data.erase(args[1]);
		return true;
	}

	bool stdmap_minimal::notify_hset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		lyramilk::threading::mutex_sync _(lock.w());
		data[args[1]][args[2]] = args[3].str();
		return true;
	}

	bool stdmap_minimal::notify_hdel(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		lyramilk::threading::mutex_sync _(lock.w());
		data[args[1]].erase(args[2]);
		return true;
	}

	stdmap_minimal::stdmap_minimal()
	{
	}

	stdmap_minimal::~stdmap_minimal()
	{
	}

	bool stdmap_minimal::get_sync_info(const lyramilk::data::string& masterid,lyramilk::data::string* replid,lyramilk::data::uint64* offset) const
	{
		return false;
	}

	bool stdmap_minimal::hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		rspeed_on_read();

		lyramilk::threading::mutex_sync _(lock.r());
		table_type::const_iterator it = data.find(key);
		if(it == data.end()) return false;
		datamap_type::const_iterator subit = it->second.find(field);
		if(subit==it->second.end()) return false;
		return true;
	}

	lyramilk::data::string stdmap_minimal::hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		rspeed_on_read();

		lyramilk::threading::mutex_sync _(lock.r());
		table_type::const_iterator it = data.find(key);
		if(it == data.end()) return "";
		datamap_type::const_iterator subit = it->second.find(field);
		if(subit==it->second.end()) return "";
		return subit->second;
	}

	lyramilk::data::stringdict stdmap_minimal::hgetall(const lyramilk::data::string& key) const
	{
		rspeed_on_read();

		lyramilk::data::stringdict m;
		lyramilk::threading::mutex_sync _(lock.r());
		table_type::const_iterator it = data.find(key);
		if(it == data.end()) return m;
		m = it->second;
		return m;
	}
}}
