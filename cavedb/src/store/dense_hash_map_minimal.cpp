#include "dense_hash_map_minimal.h"

#ifdef Z_HAVE_SPARSEHASH
	#include <google/sparse_hash_map> 
	#include <google/dense_hash_map> 
#else
	#include <sparsehash/sparse_hash_map> 
	#include <sparsehash/dense_hash_map> 
#endif

typedef std::tr1::unordered_map<std::string,std::string> datamap_type;
typedef google::dense_hash_map<std::string,datamap_type > table_type;

namespace lyramilk{ namespace cave
{
	bool dense_hash_map_minimal::notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata)
	{
		return true;
	}

	bool dense_hash_map_minimal::notify_flushdb(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::threading::mutex_sync _(lock.w());
		data.clear();
		return true;
	}

	bool dense_hash_map_minimal::notify_flushall(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::threading::mutex_sync _(lock.w());
		data.clear();
		return true;
	}

	bool dense_hash_map_minimal::notify_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::threading::mutex_sync _(lock.w());
		data.erase(args[1].str());
		return true;
	}

	bool dense_hash_map_minimal::notify_move(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return false;
	}

	bool dense_hash_map_minimal::notify_pexpireat(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return false;
	}

	bool dense_hash_map_minimal::notify_persist(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return false;
	}

	bool dense_hash_map_minimal::notify_rename(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::threading::mutex_sync _(lock.w());
		data[args[2]] = data[args[1]];
		data.erase(args[1]);
		return true;
	}

	bool dense_hash_map_minimal::notify_hset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::threading::mutex_sync _(lock.w());
		data[args[1]][args[2]] = args[3].str();
		return true;
	}

	bool dense_hash_map_minimal::notify_hdel(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::threading::mutex_sync _(lock.w());
		data[args[1]].erase(args[2]);
		return true;
	}

	dense_hash_map_minimal::dense_hash_map_minimal()
	{
		data = new table_type;
		reinterpret_cast<table_type*>(data)->set_empty_key("");
	}

	dense_hash_map_minimal::~dense_hash_map_minimal()
	{
		delete reinterpret_cast<table_type*>(this->data);
	}

	bool dense_hash_map_minimal::get_sync_info(lyramilk::data::string* replid,lyramilk::data::uint64* offset) const
	{
		return false;
	}

	bool dense_hash_map_minimal::hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		rspeed_on_read();

		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::threading::mutex_sync _(lock.r());
		table_type::const_iterator it = data.find(key);
		if(it == data.end()) return false;
		datamap_type::const_iterator subit = it->second.find(field);
		if(subit==it->second.end()) return false;
		return true;
	}

	lyramilk::data::string dense_hash_map_minimal::hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		rspeed_on_read();

		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::threading::mutex_sync _(lock.r());
		table_type::const_iterator it = data.find(key);
		if(it == data.end()) return "";
		datamap_type::const_iterator subit = it->second.find(field);
		if(subit==it->second.end()) return "";
		return subit->second;
	}

	lyramilk::data::stringdict dense_hash_map_minimal::hgetall(const lyramilk::data::string& key) const
	{
		rspeed_on_read();

		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::data::stringdict m;
		lyramilk::threading::mutex_sync _(lock.r());
		table_type::const_iterator it = data.find(key);
		if(it == data.end()) return m;
		datamap_type::const_iterator subit = it->second.begin();
		for(;subit!=it->second.end();++subit){
			m[subit->first] = subit->second;
		}
		return m;
	}
}}
