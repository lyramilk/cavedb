#include "cavedb.h"
#include "redis_to_leveldb.h"
#include <set>

namespace lyramilk{ namespace cave
{
	void static redis_sadd(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		std::set<lyramilk::data::string> norepeat;
		for(unsigned int i=2;i<args.size();++i){
			lyramilk::data::string value = args[i];
			norepeat.insert(value);
		}

		lyramilk::data::uint64 scount = rh.get_size(key);

		std::set<lyramilk::data::string>::iterator it = norepeat.begin();
		for(;it!=norepeat.end();++it){
			lyramilk::data::string value = *it;

			std::string data_key = rh.encode_set_data_key(key,value);
			std::string eoldvalue;
			leveldb::Status ldbs = rh.ldb->Get(rh.ropt,data_key,&eoldvalue);
			if(ldbs.IsNotFound()){
				++scount;
			}
			batch.Put(data_key,leveldb::Slice());
		}
		rh.set_size(batch,key,scount);
	}
	void static redis_sdiffstore(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		std::map<lyramilk::data::string,unsigned int> storemap;
		lyramilk::data::string destkey = args[1];
		for(std::size_t i=2;i<args.size();++i){
			lyramilk::data::string srckey = args[i].str();
			leveldb::ReadOptions ropt;

			leveldb_iterator it(rh.ldb->NewIterator(ropt));
			redis_leveldb_key k(rh.dbprefix(-1));
			k.append_char(redis_leveldb_key::KT_SET);
			k.append_string(srckey.c_str(),srckey.size());
			k.append_char('v');
			std::string prefix = k;
			prefix.push_back(redis_leveldb_key::KEY_STR);
			if(it) for(it->Seek(prefix);it->Valid();it->Next()){
				leveldb::Slice datakey = it->key();
				if(!datakey.starts_with(prefix)) break;
				datakey.remove_prefix(prefix.size() + sizeof(lyramilk::data::uint32));
				if(i == 2){
					++storemap[lyramilk::data::str(datakey.ToString())];
				}else{
					storemap.erase(lyramilk::data::str(datakey.ToString()));
				}
			}
		}

		/*删除destkey*/{
			leveldb::ReadOptions ropt;

			const unsigned char types[] = {redis_leveldb_key::KT_BASE,redis_leveldb_key::KT_STRING,redis_leveldb_key::KT_ZSET,redis_leveldb_key::KT_SET,redis_leveldb_key::KT_LIST,redis_leveldb_key::KT_HASH};
			for(unsigned int i=0;i<sizeof(types);++i){
				redis_leveldb_key k(rh.dbprefix(-1));
				k.append_char(types[i]);
				k.append_string(destkey.c_str(),destkey.size());
				std::string prefix = k;
				leveldb_iterator it(rh.ldb->NewIterator(ropt));
				if(it) for(it->Seek(prefix);it->Valid();it->Next()){
					if(!it->key().starts_with(prefix)) break;
					batch.Delete(it->key());
				}
			}
		}

		lyramilk::data::uint64 scount = rh.get_size(destkey);
		std::map<lyramilk::data::string,unsigned int>::iterator it = storemap.begin();
		for(;it!=storemap.end();++it){
			if(it->second  > 0){
				std::string data_key = rh.encode_set_data_key(destkey,it->first);
				batch.Put(data_key,leveldb::Slice());
				++scount;
			}
		}
		rh.set_size(batch,destkey,scount);
	}
	void static redis_sinterstore(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		std::map<lyramilk::data::string,unsigned int> storemap;
		lyramilk::data::string destkey = args[1];
		for(std::size_t i=2;i<args.size();++i){
			lyramilk::data::string srckey = args[i].str();
			leveldb::ReadOptions ropt;

			leveldb_iterator it(rh.ldb->NewIterator(ropt));
			redis_leveldb_key k(rh.dbprefix(-1));
			k.append_char(redis_leveldb_key::KT_SET);
			k.append_string(srckey.c_str(),srckey.size());
			k.append_char('v');
			std::string prefix = k;
			prefix.push_back(redis_leveldb_key::KEY_STR);
			if(it) for(it->Seek(prefix);it->Valid();it->Next()){
				leveldb::Slice datakey = it->key();
				if(!datakey.starts_with(prefix)) break;
				datakey.remove_prefix(prefix.size() + sizeof(lyramilk::data::uint32));
				++storemap[datakey.ToString()];
			}
		}

		/*删除destkey*/{
			leveldb::ReadOptions ropt;

			const unsigned char types[] = {redis_leveldb_key::KT_BASE,redis_leveldb_key::KT_STRING,redis_leveldb_key::KT_ZSET,redis_leveldb_key::KT_SET,redis_leveldb_key::KT_LIST,redis_leveldb_key::KT_HASH};
			for(unsigned int i=0;i<sizeof(types);++i){
				redis_leveldb_key k(rh.dbprefix(-1));
				k.append_char(types[i]);
				k.append_string(destkey.c_str(),destkey.size());
				std::string prefix = k;
				leveldb_iterator it(rh.ldb->NewIterator(ropt));
				if(it) for(it->Seek(prefix);it->Valid();it->Next()){
					if(!it->key().starts_with(prefix)) break;
					batch.Delete(it->key());
				}
			}
		}


		unsigned int setcount = args.size() - 2;

		lyramilk::data::uint64 scount = rh.get_size(destkey);
		std::map<lyramilk::data::string,unsigned int>::iterator it = storemap.begin();
		for(;it!=storemap.end();++it){
			if(it->second == setcount){
				std::string data_key = rh.encode_set_data_key(destkey,it->first);
				batch.Put(data_key,leveldb::Slice());
				++scount;
			}
		}
		rh.set_size(batch,destkey,scount);
	}
	void static redis_smove(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string srckey = args[1];
		lyramilk::data::string dstkey = args[2];
		lyramilk::data::string member = args[3];

		std::string data_key = rh.encode_set_data_key(srckey,member);
		std::string eoldvalue;
		leveldb::Status ldbs = rh.ldb->Get(rh.ropt,data_key,&eoldvalue);
		if(ldbs.ok()){
			batch.Delete(data_key);
			rh.decr_size(batch,srckey);
		}

		std::string data_dstkey = rh.encode_set_data_key(dstkey,member);
		ldbs = rh.ldb->Get(rh.ropt,data_dstkey,&eoldvalue);
		if(ldbs.IsNotFound()){
			batch.Put(data_dstkey,leveldb::Slice());
			rh.incr_size(batch,dstkey);
		}
	}
	void static redis_srem(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		std::set<lyramilk::data::string> norepeat;
		for(unsigned int i=2;i<args.size();++i){
			lyramilk::data::string value = args[i];
			norepeat.insert(value);
		}

		lyramilk::data::uint64 scount = rh.get_size(key);

		std::set<lyramilk::data::string>::iterator it = norepeat.begin();
		for(;it!=norepeat.end();++it){
			lyramilk::data::string value = *it;

			std::string data_key = rh.encode_set_data_key(key,value);
			std::string eoldvalue;
			leveldb::Status ldbs = rh.ldb->Get(rh.ropt,data_key,&eoldvalue);
			if(ldbs.ok()){
				--scount;
				batch.Delete(data_key);
			}
		}
		rh.set_size(batch,key,scount);
	}
	void static redis_sunionstore(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		std::map<lyramilk::data::string,unsigned int> storemap;
		lyramilk::data::string destkey = args[1];
		for(std::size_t i=2;i<args.size();++i){
			lyramilk::data::string srckey = args[i].str();
			leveldb::ReadOptions ropt;

			leveldb_iterator it(rh.ldb->NewIterator(ropt));
			redis_leveldb_key k(rh.dbprefix(-1));
			k.append_char(redis_leveldb_key::KT_SET);
			k.append_string(srckey.c_str(),srckey.size());
			k.append_char('v');
			std::string prefix = k;
			prefix.push_back(redis_leveldb_key::KEY_STR);
			if(it) for(it->Seek(prefix);it->Valid();it->Next()){
				leveldb::Slice datakey = it->key();
				if(!datakey.starts_with(prefix)) break;
				datakey.remove_prefix(prefix.size() + sizeof(lyramilk::data::uint32));
				++storemap[datakey.ToString()];
			}
		}

		/*删除destkey*/{
			leveldb::ReadOptions ropt;

			const unsigned char types[] = {redis_leveldb_key::KT_BASE,redis_leveldb_key::KT_STRING,redis_leveldb_key::KT_ZSET,redis_leveldb_key::KT_SET,redis_leveldb_key::KT_LIST,redis_leveldb_key::KT_HASH};
			for(unsigned int i=0;i<sizeof(types);++i){
				redis_leveldb_key k(rh.dbprefix(-1));
				k.append_char(types[i]);
				k.append_string(destkey.c_str(),destkey.size());
				std::string prefix = k;
				leveldb_iterator it(rh.ldb->NewIterator(ropt));
				if(it) for(it->Seek(prefix);it->Valid();it->Next()){
					if(!it->key().starts_with(prefix)) break;
					batch.Delete(it->key());
				}
			}
		}

		lyramilk::data::uint64 scount = rh.get_size(destkey);
		std::map<lyramilk::data::string,unsigned int>::iterator it = storemap.begin();
		for(;it!=storemap.end();++it){
			if(it->second > 0){
				std::string data_key = rh.encode_set_data_key(destkey,it->first);
				batch.Put(data_key,leveldb::Slice());
				++scount;
			}
		}
		rh.set_size(batch,destkey,scount);	}

	static __attribute__ ((constructor)) void __init()
	{
		cavedb_redis_commands::instance()->define("sadd",redis_sadd);
		cavedb_redis_commands::instance()->define("sdiffstore",redis_sdiffstore);
		cavedb_redis_commands::instance()->define("sinterstore",redis_sinterstore);
		cavedb_redis_commands::instance()->define("smove",redis_smove);
		cavedb_redis_commands::instance()->define("srem",redis_srem);
		cavedb_redis_commands::instance()->define("sunionstore",redis_sunionstore);
	}

	bool database::sscan(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,set_call_back cbk,void* userdata)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		leveldb::ReadOptions ropt;
		leveldb_iterator it(rh.ldb->NewIterator(ropt));

		redis_leveldb_key k(rh.dbprefix(dbid));
		k.append_char(redis_leveldb_key::KT_SET);
		k.append_string(key.c_str(),key.size());
		k.append_char('v');
		std::string prefix = k;
		prefix.push_back(redis_leveldb_key::KEY_STR);
		if(it) for(it->Seek(prefix);it->Valid();it->Next()){
			leveldb::Slice datakey = it->key();
			if(!datakey.starts_with(prefix)) break;
			datakey.remove_prefix(prefix.size() + sizeof(lyramilk::data::uint32));
			if(!cbk(key,datakey,userdata)) return false;
		}
		return true;
	}


	bool database::smembers(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,set_call_back cbk,void* userdata)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		leveldb::ReadOptions ropt;
		leveldb_iterator it(rh.ldb->NewIterator(ropt));

		redis_leveldb_key k(rh.dbprefix(dbid));
		k.append_char(redis_leveldb_key::KT_SET);
		k.append_string(key.c_str(),key.size());
		k.append_char('v');
		std::string prefix = k;
		prefix.push_back(redis_leveldb_key::KEY_STR);
		if(it) for(it->Seek(prefix);it->Valid();it->Next()){
			leveldb::Slice datakey = it->key();
			if(!datakey.starts_with(prefix)) break;
			datakey.remove_prefix(prefix.size() + sizeof(lyramilk::data::uint32));
			if(!cbk(key,datakey,userdata)) return false;
		}
		return true;
	}

	bool database::sismember(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& value)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		std::string data_key = rh.encode_set_data_key(key,value,dbid);
		std::string eoldvalue;
		leveldb::Status ldbs = rh.ldb->Get(rh.ropt,data_key,&eoldvalue);
		return !ldbs.IsNotFound();
	}


	bool database::sdiff(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& key2,set_call_back cbk,void* userdata)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		leveldb::ReadOptions ropt;

		std::map<lyramilk::data::string,unsigned int> storemap;
		lyramilk::data::strings args;
		args.push_back(key);
		args.push_back(key2);
		for(std::size_t i=0;i<args.size();++i){
			lyramilk::data::string srckey = args[i];

			leveldb_iterator it(rh.ldb->NewIterator(ropt));
			redis_leveldb_key k(rh.dbprefix(-1));
			k.append_char(redis_leveldb_key::KT_SET);
			k.append_string(srckey.c_str(),srckey.size());
			k.append_char('v');
			std::string prefix = k;
			prefix.push_back(redis_leveldb_key::KEY_STR);
			if(it) for(it->Seek(prefix);it->Valid();it->Next()){
				leveldb::Slice datakey = it->key();
				if(!datakey.starts_with(prefix)) break;
				datakey.remove_prefix(prefix.size() + sizeof(lyramilk::data::uint32));

				if(i == 0){
					storemap[lyramilk::data::str(datakey.ToString())] = 0;
				}else{
					storemap.erase(lyramilk::data::str(datakey.ToString()));
				}
			}
		}

		std::map<lyramilk::data::string,unsigned int>::iterator it = storemap.begin();
		for(;it!=storemap.end();++it){
			leveldb::Slice data(it->first.c_str(),it->first.size());
			if(!cbk(key,data,userdata)) return false;
		}
		return false;
	}

	bool database::sinter(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& key2,set_call_back cbk,void* userdata)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		leveldb::ReadOptions ropt;

		std::map<lyramilk::data::string,unsigned int> storemap;
		lyramilk::data::strings args;
		args.push_back(key);
		args.push_back(key2);
		for(std::size_t i=0;i<args.size();++i){
			lyramilk::data::string srckey = args[i];

			leveldb_iterator it(rh.ldb->NewIterator(ropt));
			redis_leveldb_key k(rh.dbprefix(-1));
			k.append_char(redis_leveldb_key::KT_SET);
			k.append_string(srckey.c_str(),srckey.size());
			k.append_char('v');
			std::string prefix = k;
			prefix.push_back(redis_leveldb_key::KEY_STR);
			if(it) for(it->Seek(prefix);it->Valid();it->Next()){
				leveldb::Slice datakey = it->key();
				if(!datakey.starts_with(prefix)) break;
				datakey.remove_prefix(prefix.size() + sizeof(lyramilk::data::uint32));
				++storemap[datakey.ToString()];
			}
		}

		std::map<lyramilk::data::string,unsigned int>::iterator it = storemap.begin();
		for(;it!=storemap.end();++it){
			if(it->second == 2){
				leveldb::Slice data(it->first.c_str(),it->first.size());
				if(!cbk(key,data,userdata)) return false;
			}
		}
		return false;
	}

	bool database::sunion(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& key2,set_call_back cbk,void* userdata)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		leveldb::ReadOptions ropt;

		std::map<lyramilk::data::string,unsigned int> storemap;
		lyramilk::data::strings args;
		args.push_back(key);
		args.push_back(key2);
		for(std::size_t i=0;i<args.size();++i){
			lyramilk::data::string srckey = args[i];

			leveldb_iterator it(rh.ldb->NewIterator(ropt));
			redis_leveldb_key k(rh.dbprefix(-1));
			k.append_char(redis_leveldb_key::KT_SET);
			k.append_string(srckey.c_str(),srckey.size());
			k.append_char('v');
			std::string prefix = k;
			prefix.push_back(redis_leveldb_key::KEY_STR);
			if(it) for(it->Seek(prefix);it->Valid();it->Next()){
				leveldb::Slice datakey = it->key();
				if(!datakey.starts_with(prefix)) break;
				datakey.remove_prefix(prefix.size() + sizeof(lyramilk::data::uint32));
				++storemap[datakey.ToString()];
			}
		}

		std::map<lyramilk::data::string,unsigned int>::iterator it = storemap.begin();
		for(;it!=storemap.end();++it){
			if(it->second > 0){
				leveldb::Slice data(it->first.c_str(),it->first.size());
				if(!cbk(key,data,userdata)) return false;
			}
		}
		return false;
	}


	lyramilk::data::uint64 database::scard(lyramilk::data::uint64 dbid,const lyramilk::data::string& key)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		return rh.get_size(key,dbid);
	}

}}
