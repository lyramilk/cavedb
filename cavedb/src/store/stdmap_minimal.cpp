#include "stdmap_minimal.h"
#include "slave_redis.h"
#include "slave_ssdb.h"


namespace lyramilk{ namespace cave
{
	bool stdmap_minimal::notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
	{
		return true;
	}

	void stdmap_minimal::notify_flushdb(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		lyramilk::threading::mutex_sync _(lock.w());
		data.clear();
	}

	void stdmap_minimal::notify_flushall(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		lyramilk::threading::mutex_sync _(lock.w());
		data.clear();
	}

	void stdmap_minimal::notify_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		lyramilk::threading::mutex_sync _(lock.w());
		data.erase(args[1].str());
	}

	void stdmap_minimal::notify_move(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
	}

	void stdmap_minimal::notify_pexpireat(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		lyramilk::threading::mutex_sync _(lock.r());
		std::map<std::string,mapeddata>::iterator it = data.find(args[1]);
		it->second.expire = args[1];
	}

	void stdmap_minimal::notify_persist(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		lyramilk::threading::mutex_sync _(lock.r());
		std::map<std::string,mapeddata>::iterator it = data.find(args[1]);
		it->second.expire = 0;
	}

	void stdmap_minimal::notify_rename(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		lyramilk::threading::mutex_sync _(lock.w());
		data[args[2]] = data[args[1]];
		data.erase(args[1]);
	}

	void stdmap_minimal::notify_hset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		std::map<std::string,mapeddata>::iterator it;
		{
			lyramilk::threading::mutex_sync _(lock.r());
			it = data.find(args[1]);
		}
		if(it == data.end()){
			lyramilk::threading::mutex_sync _(lock.w());
			data[args[1] ].data[args[2] ] = args[3].str();
		}else{
			lyramilk::threading::mutex_sync _(it->second.lock.w());
			it->second.data[args[2]] = args[3].str();
		}
	}

	void stdmap_minimal::notify_hdel(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		std::map<std::string,mapeddata>::iterator it;
		{
			lyramilk::threading::mutex_sync _(lock.r());
			it = data.find(args[1]);
		}
		if(it != data.end()){
			lyramilk::threading::mutex_sync _(it->second.lock.w());
			it->second.data.erase(args[2]);
		}
	}

	stdmap_minimal::stdmap_minimal()
	{
	}

	stdmap_minimal::~stdmap_minimal()
	{
	}

	lyramilk::data::string stdmap_minimal::hget(const lyramilk::data::string& key,const lyramilk::data::string& field)
	{
		std::map<std::string,mapeddata>::iterator it;
		{
			lyramilk::threading::mutex_sync _(lock.r());
			it = data.find(key);
		}
		if(it == data.end()) return "";
		lyramilk::threading::mutex_sync _(it->second.lock.r());
		std::map<std::string,std::string>::const_iterator subit = it->second.data.find(field);
		return subit->second;
	}

	lyramilk::data::var::map stdmap_minimal::hgetall(const lyramilk::data::string& key)
	{
		std::map<std::string,mapeddata>::iterator it;
		{
			lyramilk::threading::mutex_sync _(lock.r());
			it = data.find(key);
		}
		if(it == data.end()) return lyramilk::data::var::map();

		lyramilk::threading::mutex_sync _(it->second.lock.r());
		std::map<std::string,std::string>::const_iterator subit = it->second.data.begin();

		lyramilk::data::var::map m;
		for(;subit!=it->second.data.end();++subit){
			m[subit->first] = subit->second;
		
		}

		return m;
	}

}}
