#include "cavedb.h"
#include "redis_to_leveldb.h"
#include <stdlib.h>

namespace lyramilk{ namespace cave
{
	static leveldb::ReadOptions ropt;
	static leveldb::WriteOptions wopt;


	void static redis_hset(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::string field = args[2];
		lyramilk::data::string value = args[3];

		std::string data_key = rh.encode_hashmap_data_key(key,field);
		std::string data;
		leveldb::Status ldbs = rh.ldb->Get(ropt,data_key,&data);
		if(ldbs.IsNotFound()){
			batch.Put(data_key,lyramilk::data::str(value));
		}else{
			rh.incr_size(batch,key);
			batch.Put(data_key,lyramilk::data::str(value));
		}
	}

	void static redis_hdel(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::string field = args[2];

		std::string data_key = rh.encode_hashmap_data_key(key,field);
		std::string data;
		leveldb::Status ldbs = rh.ldb->Get(ropt,data_key,&data);
		if(ldbs.ok()){
			rh.decr_size(batch,key);
			batch.Delete(data_key);
		}
	}

	void static redis_hincrby(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::string field = args[2];
		lyramilk::data::int64 value = args[3];

		std::string data_key = rh.encode_hashmap_data_key(key,field);
		std::string data;
		leveldb::Status ldbs = rh.ldb->Get(ropt,data_key,&data);
		if(ldbs.IsNotFound()){
			batch.Put(data_key,"1");
		}else{
			rh.incr_size(batch,key);
			char *p;
			lyramilk::data::int64 oldvalue = strtoll(data.c_str(),&p,10) + value;
			char buff[64];
			int i = snprintf(buff,64,"%lld",oldvalue);
			batch.Put(data_key,leveldb::Slice(buff,i));
		}
	}

	void static redis_hmset(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::uint64 hcount = rh.get_size(key);
		for(std::size_t i=2;i<args.size();i+=2){
			lyramilk::data::string field = args[i];
			lyramilk::data::string value = args[i+1];

			std::string data_key = rh.encode_hashmap_data_key(key,field);
			std::string data;
			leveldb::Status ldbs = rh.ldb->Get(ropt,data_key,&data);
			if(ldbs.IsNotFound()){
				batch.Put(data_key,lyramilk::data::str(value));
			}else{
				++hcount;
				batch.Put(data_key,lyramilk::data::str(value));
			}
		}
		rh.set_size(batch,key,hcount);
	}

	static __attribute__ ((constructor)) void __init()
	{
		cavedb_redis_commands::instance()->define("hset",redis_hset);
		cavedb_redis_commands::instance()->define("hsetnx",redis_hset);
		cavedb_redis_commands::instance()->define("hdel",redis_hdel);
		cavedb_redis_commands::instance()->define("hincrby",redis_hincrby);
		cavedb_redis_commands::instance()->define("hmset",redis_hmset);
	}

	bool database::hscan(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,hashmap_call_back cbk,void* userdata)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		leveldb::ReadOptions ropt;
		leveldb_iterator it(rh.ldb->NewIterator(ropt));

		redis_leveldb_key k(rh.dbprefix(dbid));
		k.append_char(redis_leveldb_key::KT_HASH);
		k.append_string(key.c_str(),key.size());
		k.append_char('f');
		std::string prefix = k;
		prefix.push_back(redis_leveldb_key::KEY_STR);

		if(it) for(it->Seek(prefix);it->Valid();it->Next()){
			leveldb::Slice datakey = it->key();
			if(!datakey.starts_with(prefix)) break;
			datakey.remove_prefix(prefix.size() + sizeof(lyramilk::data::uint32));
			leveldb::Slice valuekey = it->value();

			if(!cbk(key,datakey,valuekey,userdata)) return false;
		}
		return true;
	}

	bool database::hexists(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& field)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		std::string data_key = rh.encode_hashmap_data_key(key,field,dbid);
		std::string data;
		leveldb::Status ldbs = rh.ldb->Get(ropt,data_key,&data);
		return !ldbs.IsNotFound();
	}

	lyramilk::data::string database::hget(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& field)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		std::string data_key = rh.encode_hashmap_data_key(key,field,dbid);
		std::string data;
		leveldb::Status ldbs = rh.ldb->Get(ropt,data_key,&data);
		if(ldbs.ok()){
			return lyramilk::data::str(data);
		}
		return "";
	}

	bool static hashmap_call_back_hgetall(const lyramilk::data::string& key,leveldb::Slice& field,leveldb::Slice& value,void* userdata)
	{
		lyramilk::data::var::map* mm = reinterpret_cast<lyramilk::data::var::map*>(userdata);
		mm->operator[](lyramilk::data::string(field.data(),field.size())) = lyramilk::data::string(value.data(),value.size());
		return true;
	}

	lyramilk::data::var::map database::hgetall(lyramilk::data::uint64 dbid,const lyramilk::data::string& key)
	{
		lyramilk::data::var::map mm;
		hscan(dbid,key,hashmap_call_back_hgetall,&mm);
		return mm;
	}

	lyramilk::data::uint64 database::hlen(lyramilk::data::uint64 dbid,const lyramilk::data::string& key)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		return rh.get_size(key,dbid);
	}


}}
