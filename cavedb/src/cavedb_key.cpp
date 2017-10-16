#include "cavedb.h"
#include "rdb.h"
#include "redis_to_leveldb.h"
#include <libmilk/multilanguage.h>
#include <libmilk/log.h>
#include <libmilk/testing.h>

namespace lyramilk{ namespace cave
{
	class rdbparser_restore:public rdb
	{
		slave* ps;
	  public:
		rdbparser_restore(slave* s)
		{
			ps = s;
		}
		virtual ~rdbparser_restore()
		{
		}

		virtual void notify_hset(const lyramilk::data::string& key,const lyramilk::data::string& field,const lyramilk::data::var& value)
		{
			lyramilk::data::var::array ar;
			ar.push_back("hset");
			ar.push_back(key);
			ar.push_back(field);
			ar.push_back(value);
			ps->notify_command("?",0,ar);
		}
		virtual void notify_zadd(const lyramilk::data::string& key,const lyramilk::data::var& value,double score)
		{
			lyramilk::data::var::array ar;
			ar.push_back("zadd");
			ar.push_back(key);
			ar.push_back(score);
			ar.push_back(value);
			ps->notify_command("?",0,ar);
		}
		virtual void notify_set(const lyramilk::data::string& key,const lyramilk::data::string& value)
		{
			lyramilk::data::var::array ar;
			ar.push_back("set");
			ar.push_back(key);
			ar.push_back(value);
			ps->notify_command("?",0,ar);
		}

		virtual void notify_rpush(const lyramilk::data::string& key,const lyramilk::data::string& item)
		{
			lyramilk::data::var::array ar;
			ar.push_back("rpush");
			ar.push_back(key);
			ar.push_back(item);
			ps->notify_command("?",0,ar);
		}

		virtual void notify_sadd(const lyramilk::data::string& key,const lyramilk::data::string& value)
		{
			lyramilk::data::var::array ar;
			ar.push_back("sadd");
			ar.push_back(key);
			ar.push_back(value);
			ps->notify_command("?",0,ar);
		}
	};

	void static redis_del(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		for(std::size_t index=1;index<args.size();++index){
			lyramilk::data::string key = args[index];
			leveldb::ReadOptions ropt;
			leveldb::WriteOptions wopt;

			const char types[] = {redis_leveldb_key::KT_BASE,redis_leveldb_key::KT_STRING,redis_leveldb_key::KT_ZSET,redis_leveldb_key::KT_SET,redis_leveldb_key::KT_LIST,redis_leveldb_key::KT_HASH};
			for(unsigned int i=0;i<sizeof(types);++i){
				redis_leveldb_key k(rh.dbprefix(-1));
				k.append_type(types[i]);
				k.append_string(key.c_str(),key.size());
				std::string prefix = k;
				prefix.push_back(redis_leveldb_key::KEY_STR);

				leveldb_iterator it(rh.ldb->NewIterator(ropt));
				if(it) for(it->Seek(prefix);it->Valid();it->Next()){
					if(!it->key().starts_with(prefix)) break;
					rh.ldb->Delete(wopt,it->key());
				}
			}
		}
	}
	void static redis_move(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::uint64 newdbid = args[1];
		leveldb::ReadOptions ropt;

		std::string newdbprefix = rh.dbprefix(newdbid);

		const char types[] = {redis_leveldb_key::KT_BASE,redis_leveldb_key::KT_STRING,redis_leveldb_key::KT_ZSET,redis_leveldb_key::KT_SET,redis_leveldb_key::KT_LIST,redis_leveldb_key::KT_HASH};
		for(unsigned int i=0;i<sizeof(types);++i){
			redis_leveldb_key k(rh.dbprefix(-1));
			k.append_type(types[i]);
			k.append_string(key.c_str(),key.size());
			std::string prefix = k;
			prefix.push_back(redis_leveldb_key::KEY_STR);

			leveldb_iterator it(rh.ldb->NewIterator(ropt));
			if(it) for(it->Seek(prefix);it->Valid();it->Next()){
				if(!it->key().starts_with(prefix)) break;
				batch.Delete(it->key());
				leveldb::Slice skey = it->key();
				skey.remove_prefix(newdbprefix.size());
				std::string str = newdbprefix;
				str.append(skey.data(),skey.size());
				batch.Put(str,it->value());
			}
		}
	}

	void static redis_restore(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::uint64 ttl = args[2];
		lyramilk::data::string data = args[3];
		args[3] = "<序列化值>";//为了日志显示起来顺眼
		lyramilk::data::stringstream is(data);
		slave* slv = (slave*) reinterpret_cast<slave*>(rh.userdata);
		rdbparser_restore r(slv);
		r.restore(is,rh.get_dbid(),ttl,key);
	}

	void static redis_expire(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::uint64 ttl = args[2];
		
		rh.set_pttl(batch,key,rh.mstime() + 1000 * ttl);
	}

	void static redis_expireat(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::uint64 ttl = args[2];
		
		rh.set_pttl(batch,key,1000 * ttl);
	}

	void static redis_pexpire(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::uint64 pttl = args[2];
		
		rh.set_pttl(batch,key,rh.mstime() + pttl);
	}

	void static redis_pexpireat(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::uint64 pttl = args[2];
		
		rh.set_pttl(batch,key,pttl);
	}

	void static redis_persist(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		std::string ttl_key = rh.encode_size_key(key);
		batch.Delete(ttl_key);
	}

	void static redis_rename(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::string key2 = args[2];
		leveldb::ReadOptions ropt;

		const char types[] = {redis_leveldb_key::KT_BASE,redis_leveldb_key::KT_STRING,redis_leveldb_key::KT_ZSET,redis_leveldb_key::KT_SET,redis_leveldb_key::KT_LIST,redis_leveldb_key::KT_HASH};
		for(unsigned int i=0;i<sizeof(types);++i){
			redis_leveldb_key k(rh.dbprefix(-1));
			k.append_type(types[i]);
			k.append_string(key.c_str(),key.size());
			std::string prefix = k;
			prefix.push_back(redis_leveldb_key::KEY_STR);

			redis_leveldb_key k2(rh.dbprefix(-1));
			k.append_type(types[i]);
			k.append_string(key2.c_str(),key2.size());
			std::string finkey = k2;
			finkey.push_back(redis_leveldb_key::KEY_STR);

			leveldb_iterator it(rh.ldb->NewIterator(ropt));
			if(it) for(it->Seek(prefix);it->Valid();it->Next()){
				if(!it->key().starts_with(prefix)) break;
				batch.Delete(it->key());
				leveldb::Slice skey = it->key();
				skey.remove_prefix(prefix.size());
				batch.Put(finkey + skey.ToString(),it->value());
			}
		}
	}

	static __attribute__ ((constructor)) void __init()
	{
		cavedb_redis_commands::instance()->define("del",redis_del);
		cavedb_redis_commands::instance()->define("move",redis_move);
		cavedb_redis_commands::instance()->define("restore",redis_restore);
		cavedb_redis_commands::instance()->define("expire",redis_expire);
		cavedb_redis_commands::instance()->define("expireat",redis_expireat);
		cavedb_redis_commands::instance()->define("pexpire",redis_pexpire);
		cavedb_redis_commands::instance()->define("pexpireat",redis_pexpireat);
		cavedb_redis_commands::instance()->define("persist",redis_persist);
		cavedb_redis_commands::instance()->define("rename",redis_rename);
		cavedb_redis_commands::instance()->define("renamenx",redis_rename);
	}



	lyramilk::data::int64 database::ttl(lyramilk::data::uint64 dbid,const lyramilk::data::string& key)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		lyramilk::data::int64 ret = rh.get_pttl(key,dbid);
		if(ret == 0) return -1;
		return (ret - rh.mstime()) / 1000;
	}

	lyramilk::data::int64 database::pttl(lyramilk::data::uint64 dbid,const lyramilk::data::string& key)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		lyramilk::data::int64 ret = rh.get_pttl(key,dbid);
		if(ret == 0) return -1;
		return ret - rh.mstime();
	}

}}
