#include "cavedb.h"
#include "redis_to_leveldb.h"
#include <stdlib.h>

namespace lyramilk{ namespace cave
{
	static leveldb::ReadOptions ropt;
	static leveldb::WriteOptions wopt;


	void static redis_append(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::string diffval = args[2];

		std::string data_key = rh.encode_string_key(key);
		std::string value;
		leveldb::Status ldbs = rh.ldb->Get(ropt,data_key,&value);
		if(ldbs.ok()){
			value.append(diffval.c_str(),diffval.size());
		}else{
			value.append(diffval.c_str(),diffval.size());
		}
		batch.Put(data_key,value);
	}

	void static redis_bitor(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string ops = args[1];
		enum{
			AND,OR,XOR,NOT
		}op = AND;
		if(ops == "AND"){
			op = AND;
		}else if(ops == "OR"){
			op = OR;
		}else if(ops == "XOR"){
			op = XOR;
		}else if(ops == "NOT"){
			op = NOT;
		}
		lyramilk::data::string key = args[2];

		std::string ret;

		for(std::size_t i=3;i<args.size();++i){
			lyramilk::data::string srckey = args[2];
			std::string data_key = rh.encode_string_key(key);
			std::string value;
			rh.ldb->Get(ropt,data_key,&value);
			while(ret.size() < value.size()){
				ret.push_back(0);
			}
			while(value.size() < ret.size()){
				value.push_back(0);
			}

			for(std::size_t bitindex=0;bitindex<ret.size();++bitindex){
				if(op == AND){
					ret[bitindex] &= value[bitindex];
				}else if(op == OR){
					ret[bitindex] |= value[bitindex];
				}else if(op == XOR){
					ret[bitindex] ^= value[bitindex];
				}else if(op == NOT){
					ret[bitindex] = ~value[bitindex];
				}
			}
		}
		std::string data_key = rh.encode_string_key(key);
		batch.Put(data_key,ret);
	}

	void static redis_decr(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];

		std::string data_key = rh.encode_string_key(key);
		std::string value;
		leveldb::Status ldbs = rh.ldb->Get(ropt,data_key,&value);
		long long oldvalue = 0;
		if(ldbs.ok()){
			char* p;
			oldvalue = strtoll(value.c_str(),&p,10);
		}
		--oldvalue;
		char buff[64];
		int n = snprintf(buff,64,"%llu",oldvalue);
		std::string newvalue(buff,n);
		batch.Put(data_key,newvalue);
	}

	void static redis_decrby(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::string opds = args[2];

		std::string data_key = rh.encode_string_key(key);
		std::string value;
		leveldb::Status ldbs = rh.ldb->Get(ropt,data_key,&value);
		long long oldvalue = 0;
		if(ldbs.ok()){
			char* p;
			oldvalue = strtoll(value.c_str(),&p,10);
		}
		char* p;
		long long opd = strtoll(opds.c_str(),&p,10);
		oldvalue -= opd;
		char buff[64];
		int n = snprintf(buff,64,"%llu",oldvalue);
		std::string newvalue(buff,n);
		batch.Put(data_key,newvalue);
	}
	void static redis_incr(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];

		std::string data_key = rh.encode_string_key(key);
		std::string value;
		leveldb::Status ldbs = rh.ldb->Get(ropt,data_key,&value);
		long long oldvalue = 0;
		if(ldbs.ok()){
			char* p;
			oldvalue = strtoll(value.c_str(),&p,10);
		}
		++oldvalue;
		char buff[64];
		int n = snprintf(buff,64,"%llu",oldvalue);
		std::string newvalue(buff,n);
		batch.Put(data_key,newvalue);
	}

	void static redis_incrby(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::string opds = args[2];

		std::string data_key = rh.encode_string_key(key);
		std::string value;
		leveldb::Status ldbs = rh.ldb->Get(ropt,data_key,&value);
		long long oldvalue = 0;
		if(ldbs.ok()){
			char* p;
			oldvalue = strtoll(value.c_str(),&p,10);
		}
		char* p;
		long long opd = strtoll(opds.c_str(),&p,10);
		oldvalue += opd;
		char buff[64];
		int n = snprintf(buff,64,"%llu",oldvalue);
		std::string newvalue(buff,n);
		batch.Put(data_key,newvalue);
	}

	void static redis_mset(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		for(std::size_t i=1;i<args.size();i+=2){
			lyramilk::data::string key = args[i];

			lyramilk::data::string value = args[i+1];

			std::string data_key = rh.encode_string_key(key);
			batch.Put(data_key,lyramilk::data::str(value));
		}
	}

	void static redis_psetex(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::uint64 pttl = args[2];
		lyramilk::data::string value = args[3];

		std::string data_key = rh.encode_string_key(key);
		batch.Put(data_key,lyramilk::data::str(value));
		rh.set_pttl(batch,key,rh.mstime() + pttl);
	}

	void static redis_setex(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::uint64 ttl = args[2];
		lyramilk::data::string value = args[3];

		std::string data_key = rh.encode_string_key(key);
		batch.Put(data_key,lyramilk::data::str(value));
		rh.set_pttl(batch,key,rh.mstime() + 1000 * ttl);
	}

	void static redis_set(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::string value = args[2];

		std::string data_key = rh.encode_string_key(key);
		batch.Put(data_key,lyramilk::data::str(value));
	}

	void static redis_setbit(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::uint64 offset = args[2];
		lyramilk::data::uint64 diffval = args[3];

		std::string data_key = rh.encode_string_key(key);
		std::string value;
		rh.ldb->Get(ropt,data_key,&value);

		lyramilk::data::uint64 byte_offset = offset >> 3;
		lyramilk::data::uint64 bit_offset = offset & 7;

		while(value.size() < (byte_offset + 1 + (bit_offset != 0))){
			value.push_back(0);
		}
		char& c = value[byte_offset];
		if(diffval != 0){
			c |= (1 << (7-bit_offset));
		}else{
			c &= ~(1 << (7-bit_offset)); 
		}
		batch.Put(data_key,value);
	}


	void static redis_setrange(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::uint64 offset = args[2];
		lyramilk::data::string diffval = args[3];

		std::string data_key = rh.encode_string_key(key);
		std::string value;
		rh.ldb->Get(ropt,data_key,&value);

		while(value.size() < diffval.size() + offset){
			value.push_back(0);
		}
		memcpy((char*)value.data() + offset,diffval.data(),diffval.size());
		batch.Put(data_key,value);
	}

	static __attribute__ ((constructor)) void __init()
	{
		cavedb_redis_commands::instance()->define("append",redis_append);
		cavedb_redis_commands::instance()->define("bitor",redis_bitor);
		cavedb_redis_commands::instance()->define("getset",redis_set);
		cavedb_redis_commands::instance()->define("decr",redis_decr);
		cavedb_redis_commands::instance()->define("decrby",redis_decrby);
		cavedb_redis_commands::instance()->define("incr",redis_incr);
		cavedb_redis_commands::instance()->define("incrby",redis_incrby);
		cavedb_redis_commands::instance()->define("mset",redis_mset);
		cavedb_redis_commands::instance()->define("msetnx",redis_mset);
		cavedb_redis_commands::instance()->define("psetex",redis_psetex);
		cavedb_redis_commands::instance()->define("setex",redis_setex);
		cavedb_redis_commands::instance()->define("set",redis_set);
		cavedb_redis_commands::instance()->define("setnx",redis_set);
		cavedb_redis_commands::instance()->define("setbit",redis_setbit);
		cavedb_redis_commands::instance()->define("setrange",redis_setrange);
	}


	lyramilk::data::string database::get(lyramilk::data::uint64 dbid,const lyramilk::data::string& key)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;

		std::string data_key = rh.encode_string_key(key);
		std::string value;
		rh.ldb->Get(ropt,data_key,&value);
		return lyramilk::data::str(value);
	}

}}
