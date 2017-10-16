#include "cavedb.h"
#include "redis_to_leveldb.h"

namespace lyramilk{ namespace cave
{
#if 0
	void static redis_rpush(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::string value = args[2];
		rh.set_type(batch,key,redis_leveldb_handler::kt_list);
		if(!rh.incr_size(batch,key)) return;
		std::string tailseqkey = rh.encode_list_tailseq_key(key);
		std::string tailseqvalue;
		rh.ldb->Get(rh.ropt,tailseqkey,&tailseqvalue);

		lyramilk::data::uint64 tailseq = 0;
		if(tailseqvalue.empty()){
			tailseq = rh.get_list_seq(key,-1);
		}else{
			tailseq = rh.str2integer(lyramilk::data::str(tailseqvalue));
			if(tailseq == 0){
				tailseq = rh.get_list_seq(key,-1);
			}else{
				++tailseq;
			}
		}
		batch.Put(tailseqkey,lyramilk::data::str(rh.integer2fstr(tailseq)));
		batch.Put(rh.encode_list_data_key(key,tailseq),lyramilk::data::str(value));
	}

	void static redis_rpop(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		rh.set_type(batch,key,redis_leveldb_handler::kt_list);
		if(!rh.decr_size(batch,key)) return;

		leveldb_iterator it(rh.ldb->NewIterator(rh.ropt));
		std::string base = rh.encode_list_data_key_base(key);
		std::string eof = rh.encode_list_data_key_eof(key);
		it->Seek(eof);
		it->Prev();
		if(it->Valid() && it->key().starts_with(base)){
			batch.Delete(it->key());
		}
	}

	void static redis_lpush(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::string value = args[2];
		rh.set_type(batch,key,redis_leveldb_handler::kt_list);
		if(!rh.incr_size(batch,key)) return;

		std::string headseqkey = rh.encode_list_headseq_key(key);
		std::string headseqvalue;
		rh.ldb->Get(rh.ropt,headseqkey,&headseqvalue);

		lyramilk::data::uint64 headseq = 0;
		if(headseqvalue.empty()){
			headseq = rh.get_list_seq(key,0);
		}else{
			headseq = rh.str2integer(lyramilk::data::str(headseqvalue));
			if(headseq == 0){
				headseq = rh.get_list_seq(key,0);
			}else{
				--headseq;
			}
		}

		batch.Put(headseqkey,lyramilk::data::str(rh.integer2fstr(headseq)));
		batch.Put(rh.encode_list_data_key(key,headseq),lyramilk::data::str(value));
	}

	void static redis_lpop(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		rh.set_type(batch,key,redis_leveldb_handler::kt_list);
		if(!rh.decr_size(batch,key)) return;

		leveldb_iterator it(rh.ldb->NewIterator(rh.ropt));
		std::string base = rh.encode_list_data_key_base(key);
		it->Seek(base);
		if(it->Valid() && it->key().starts_with(base)){
			batch.Delete(it->key());
		}
	}

	static __attribute__ ((constructor)) void __init()
	{
		//list 太慢，不用了。
		cavedb_redis_commands::instance()->define("rpush",redis_rpush);
		cavedb_redis_commands::instance()->define("rpop",redis_rpop);
		cavedb_redis_commands::instance()->define("lpush",redis_lpush);
		cavedb_redis_commands::instance()->define("lpop",redis_lpop);
	}
#endif
}}
