#include "sparse_hash_map_minimal.h"

#include <sparsehash/sparse_hash_map> 
#include <sparsehash/dense_hash_map> 

typedef std::tr1::unordered_map<std::string,std::string> datamap_type;
typedef google::sparse_hash_map<std::string,datamap_type > table_type;

namespace lyramilk{ namespace cave
{
	bool sparse_hash_map_minimal::notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
	{
		return true;
	}

	void sparse_hash_map_minimal::notify_flushdb(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::threading::mutex_sync _(lock.w());
		data.clear();
	}

	void sparse_hash_map_minimal::notify_flushall(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::threading::mutex_sync _(lock.w());
		data.clear();
	}

	void sparse_hash_map_minimal::notify_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::threading::mutex_sync _(lock.w());
		data.erase(args[1].str());
	}

	void sparse_hash_map_minimal::notify_move(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
	}

	void sparse_hash_map_minimal::notify_pexpireat(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
	}

	void sparse_hash_map_minimal::notify_persist(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
	}

	void sparse_hash_map_minimal::notify_rename(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::threading::mutex_sync _(lock.w());
		data[args[2]] = data[args[1]];
		data.erase(args[1]);
	}

	void sparse_hash_map_minimal::notify_hset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::threading::mutex_sync _(lock.w());
		data[args[1]][args[2]] = args[3].str();
	}

	void sparse_hash_map_minimal::notify_hdel(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::threading::mutex_sync _(lock.w());
		data[args[1]].erase(args[2]);
	}

	sparse_hash_map_minimal::sparse_hash_map_minimal()
	{
		data = new table_type;
	}

	sparse_hash_map_minimal::~sparse_hash_map_minimal()
	{
		delete reinterpret_cast<table_type*>(this->data);
	}

	bool sparse_hash_map_minimal::get_sync_info(lyramilk::data::string* replid,lyramilk::data::uint64* offset) const
	{
		return false;
	}

	bool sparse_hash_map_minimal::hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::threading::mutex_sync _(lock.r());
		table_type::const_iterator it = data.find(key);
		if(it == data.end()) return false;
		datamap_type::const_iterator subit = it->second.find(field);
		if(subit==it->second.end()) return false;
		return true;
	}

	lyramilk::data::string sparse_hash_map_minimal::hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::threading::mutex_sync _(lock.r());
		table_type::const_iterator it = data.find(key);
		if(it == data.end()) return "";
		datamap_type::const_iterator subit = it->second.find(field);
		if(subit==it->second.end()) return "";
		return subit->second;
	}

	lyramilk::data::var::map sparse_hash_map_minimal::hgetall(const lyramilk::data::string& key) const
	{
		table_type& data = *reinterpret_cast<table_type*>(this->data);
		lyramilk::data::var::map m;
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
