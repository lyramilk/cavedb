#include "cavedb.h"
#include "redis_to_leveldb.h"
#include <stdlib.h>
#include <algorithm>

namespace lyramilk{ namespace cave
{
	static leveldb::ReadOptions ropt;
	static leveldb::WriteOptions wopt;



	void static redis_zadd(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		std::map<lyramilk::data::string,double> norepeat;
		for(unsigned int i=2;i<args.size();i+=2){
			double newscore = args[i];
			lyramilk::data::string value = args[i+1];
			norepeat[value] = newscore;
		}

		lyramilk::data::uint64 zcount = rh.get_size(key);

		std::map<lyramilk::data::string,double>::iterator it = norepeat.begin();
		for(;it!=norepeat.end();++it){
			double newscore = it->second;
			lyramilk::data::string value = it->first;
			//rh.set_type(batch,key,redis_leveldb_handler::kt_zset);

			std::string data_key = rh.encode_zset_data_key(key,value);
			std::string score_key = rh.encode_zset_score_key(key,newscore,value);
			std::string soldscore;
			leveldb::Status ldbs = rh.ldb->Get(rh.ropt,data_key,&soldscore);
			if(ldbs.ok()){
				double oldscore = rh.bytes2double(soldscore);
				batch.Delete(rh.encode_zset_score_key(key,oldscore,value));
			}else{
				++zcount;
			}

			batch.Put(data_key,rh.double2bytes(newscore));
			batch.Put(score_key,leveldb::Slice());
		}
		rh.set_size(batch,key,zcount);

	}

	void static redis_zincrby(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		double incrscore = args[2];
		lyramilk::data::string value = args[3];
		//rh.set_type(batch,key,redis_leveldb_handler::kt_zset);

		std::string data_key = rh.encode_zset_data_key(key,value);

		double oldscore = 0.0f;
		std::string soldscore;
		leveldb::Status ldbs = rh.ldb->Get(rh.ropt,data_key,&soldscore);
		if(ldbs.ok()){
			oldscore = rh.bytes2double(soldscore);
			batch.Delete(rh.encode_zset_score_key(key,oldscore,value));
		}else{
			rh.incr_size(batch,key);
		}

		std::string snewscore = rh.double2bytes(oldscore + incrscore);
		batch.Put(data_key,snewscore);
		batch.Put(rh.encode_zset_score_key(key,rh.bytes2double(snewscore),value),leveldb::Slice());
	}

	void static redis_zrem(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];

		std::map<lyramilk::data::string,bool> norepeat;
		for(unsigned int i=2;i<args.size();++i){
			norepeat[args[i]] = true;
		}

		lyramilk::data::uint64 zcount = rh.get_size(key);
		std::map<lyramilk::data::string,bool>::iterator it = norepeat.begin();
		for(;it!=norepeat.end();++it){
			lyramilk::data::string value = it->first;

			std::string data_key = rh.encode_zset_data_key(key,value);
			std::string soldscore;
			leveldb::Status ldbs = rh.ldb->Get(rh.ropt,data_key,&soldscore);
			if(ldbs.ok()){
				batch.Delete(rh.encode_zset_score_key(key,rh.bytes2double(soldscore),value));
				batch.Delete(data_key);
				--zcount;
			}
		}
		rh.set_size(batch,key,zcount);
	}

	void static redis_zremrangebyrank(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::int64 arg_start = args[2];
		lyramilk::data::int64 arg_stop = args[3];
		lyramilk::data::uint64 start = 0;
		lyramilk::data::uint64 stop = 0;

		lyramilk::data::uint64 count = (lyramilk::data::int64)rh.get_size(key);
		if(arg_start < 0){
			start = count + arg_start;
		}else{
			start = arg_start;
		}
		if(arg_stop < 0){
			stop = count + arg_stop;
		}else{
			stop = arg_stop;
		}
		lyramilk::data::uint64 zcount = rh.get_size(key);


		leveldb::ReadOptions ropt;
		leveldb_iterator it(rh.ldb->NewIterator(ropt));

		redis_leveldb_key k(rh.dbprefix(-1));
		k.append_type(redis_leveldb_key::KT_ZSET);
		k.append_string(key.c_str(),key.size());
		k.append_string("s",1);
		std::string prefix = k;
		prefix.push_back(redis_leveldb_key::KEY_DOUBLE);

		lyramilk::data::uint64 index = 0;
		if(it) for(it->Seek(prefix);it->Valid();it->Next()){
			leveldb::Slice datakey = it->key();
			if(!datakey.starts_with(prefix)) break;
			if(index < start) continue;
			if(index > stop) break;
			++index;
			batch.Delete(it->key());

			datakey.remove_prefix(prefix.size());
			//double s = rh.bytes2double(datakey);
			datakey.remove_prefix(sizeof(double) + 1);

			std::string data_key = rh.encode_zset_data_key(key,lyramilk::data::str(datakey.ToString()));
			batch.Delete(data_key);
			--zcount;
		}
		rh.set_size(batch,key,zcount);
	}

	void static redis_zremrangebyscore(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::string score_min_str = args[2];
		lyramilk::data::string score_max_str = args[3];

		double score_min = 0.0f;
		bool score_min_nobound = false;
		bool score_min_inf = false;

		double score_max = 0.0f;
		bool score_max_nobound = false;
		bool score_max_inf = false;

		if(score_min_str[0] == '('){
			score_min_nobound = true;
			char* p;
			score_min = strtod(score_min_str.c_str()+1,&p);
		}else if(score_min_str == "-inf"){
			score_min_inf = true;
		}else{
			char* p;
			score_min = strtod(score_min_str.c_str(),&p);
		}

		if(score_max_str[0] == '('){
			score_max_nobound = true;
			char* p;
			score_max = strtod(score_max_str.c_str()+1,&p);
		}else if(score_max_str == "+inf"){
			score_max_inf = true;
		}else{
			char* p;
			score_max = strtod(score_max_str.c_str(),&p);
		}

		lyramilk::data::uint64 zcount = rh.get_size(key);

		leveldb::ReadOptions ropt;
		leveldb_iterator it(rh.ldb->NewIterator(ropt));

		redis_leveldb_key k(rh.dbprefix(-1));
		k.append_type(redis_leveldb_key::KT_ZSET);
		k.append_string(key.c_str(),key.size());
		k.append_string("s",1);
		std::string prefix = k;
		prefix.push_back(redis_leveldb_key::KEY_DOUBLE);
		std::string key_begin;
		if(score_min_inf){
			key_begin = prefix;
		}else{
			k.append_double(score_min);
			key_begin = k;
		}

		if(it) for(it->Seek(key_begin);it->Valid();it->Next()){
			leveldb::Slice datakey = it->key();
			if(!datakey.starts_with(prefix)) break;

			datakey.remove_prefix(prefix.size());
			double s = rh.bytes2double(datakey);

			if(!score_max_inf && (s > score_max || (s == score_max && score_max_nobound)) ) break;
			if(!score_min_inf && s == score_min && score_min_nobound) continue;

			batch.Delete(it->key());
			datakey.remove_prefix(sizeof(double) + 1);

			std::string data_key = rh.encode_zset_data_key(key,lyramilk::data::str(datakey.ToString()));
			batch.Delete(data_key);
			--zcount;
		}
		rh.set_size(batch,key,zcount);
	}


	double static redis_store_min(double a,double b)
	{
		return a<b?a:b;
	}

	double static redis_store_max(double a,double b)
	{
		return a<b?b:a;
	}

	double static redis_store_sum(double a,double b)
	{
		return a+b;
	}

	void static redis_zunionstore(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string destkey = args[1];
		lyramilk::data::uint64 numkeys = args[2];

		std::vector<lyramilk::data::string> srckeys;
		std::vector<double> srcweights;
		srckeys.reserve(numkeys);
		srcweights.reserve(numkeys);

		double (*aggregate)(double,double) = redis_store_sum;
		{

			lyramilk::data::var::array::iterator it = args.begin() + 3;
			enum{
				sNOOP,
				sKEYS,
				sWEIGTHS,
				sAGGREGATE,
			}s = sKEYS;
			for(;it!=args.end();++it){
				switch(s){
				  case sKEYS:{
						if(srckeys.size() < srckeys.capacity()){
							srckeys.push_back(it->str());
						}
						if(srckeys.size() >= srckeys.capacity()){
							s = sNOOP;
						}
					}break;
				  case sNOOP:{
						lyramilk::data::string flag = it->str();
						std::transform(flag.begin(),flag.end(),flag.begin(),::tolower);
						if(flag == "weights"){
							s = sWEIGTHS;
						}else if(flag == "aggregate"){
							s = sAGGREGATE;
						}else{
							TODO();
						}
					}break;
				  case sWEIGTHS:{
						if(srcweights.size() < srcweights.capacity()){
							srcweights.push_back(*it);
						}
						if(srcweights.size() >= srcweights.capacity()){
							s = sNOOP;
						}
					}break;
				  case sAGGREGATE:{
						lyramilk::data::string flag = it->str();
						std::transform(flag.begin(),flag.end(),flag.begin(),::tolower);
						if(flag == "sum"){
							aggregate = redis_store_sum;
						}else if(flag == "min"){
							aggregate = redis_store_min;
						}else if(flag == "max"){
							aggregate = redis_store_max;
						}else{
							TODO();
						}
					}break;
				  default:
					TODO();
				}
			}
		}
		for(std::size_t i=0;i<srckeys.size();++i){
			if(srcweights.size() < i + 1){
				srcweights.push_back(1.0f);
			}
		}


		std::map<std::string,double> destvalues;
		std::map<std::string,double>::iterator vit;


		for(std::size_t i=0;i<srckeys.size();++i){
			lyramilk::data::string key = srckeys[i];
			double weights = srcweights[i];
			leveldb::ReadOptions ropt;
			leveldb_iterator it(rh.ldb->NewIterator(ropt));

			redis_leveldb_key k(rh.dbprefix(-1));
			k.append_type(redis_leveldb_key::KT_ZSET);
			k.append_string(key.c_str(),key.size());
			k.append_string("s",1);
			std::string prefix = k;
			prefix.push_back(redis_leveldb_key::KEY_DOUBLE);
			if(it) for(it->Seek(prefix);it->Valid();it->Next()){
				leveldb::Slice datakey = it->key();
				if(!datakey.starts_with(prefix)) break;
				datakey.remove_prefix(prefix.size());
				double score = rh.bytes2double(datakey);
				datakey.remove_prefix(sizeof(double) + 1);

				leveldb::Slice tabkey(key.c_str(),key.size());
				{
					std::string sdatakey = datakey.ToString();
					vit = destvalues.find(sdatakey);
					if(vit == destvalues.end()){
						destvalues[sdatakey] = score;
					}else{
						vit->second = aggregate(vit->second,score * weights);
					}
				}
			}
		}

		// redis_del
		leveldb::ReadOptions ropt;

		const unsigned char types[] = {redis_leveldb_key::KT_BASE,redis_leveldb_key::KT_STRING,redis_leveldb_key::KT_ZSET,redis_leveldb_key::KT_SET,redis_leveldb_key::KT_LIST,redis_leveldb_key::KT_HASH};
		for(unsigned int i=0;i<sizeof(types);++i){
			redis_leveldb_key k(rh.dbprefix(-1));
			k.append_type(types[i]);
			k.append_string(destkey.c_str(),destkey.size());
			std::string prefix = k;
			prefix.push_back(redis_leveldb_key::KEY_STR);

			leveldb_iterator it(rh.ldb->NewIterator(ropt));
			if(it) for(it->Seek(prefix);it->Valid();it->Next()){
				if(!it->key().starts_with(prefix)) break;
				batch.Delete(it->key());
			}
		}

		// zadd
		for(vit = destvalues.begin();vit != destvalues.end();++vit){
			double score = vit->second;
			std::string value = vit->first;
			std::string data_key = rh.encode_zset_data_key(destkey,lyramilk::data::str(value));
			std::string score_key = rh.encode_zset_score_key(destkey,score,lyramilk::data::str(value));
			batch.Put(data_key,rh.double2bytes(score));
			batch.Put(score_key,leveldb::Slice());
		}
		rh.set_size(batch,destkey,destvalues.size());
	}

	struct destvalueinfo
	{
		double score;
		lyramilk::data::uint64 ref;
	};

	void static redis_zinterstore(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		lyramilk::data::string destkey = args[1];
		lyramilk::data::uint64 numkeys = args[2];

		std::vector<lyramilk::data::string> srckeys;
		std::vector<double> srcweights;
		srckeys.reserve(numkeys);
		srcweights.reserve(numkeys);

		double (*aggregate)(double,double) = redis_store_sum;
		{

			lyramilk::data::var::array::iterator it = args.begin() + 3;
			enum{
				sNOOP,
				sKEYS,
				sWEIGTHS,
				sAGGREGATE,
			}s = sKEYS;
			for(;it!=args.end();++it){
				switch(s){
				  case sKEYS:{
						if(srckeys.size() < srckeys.capacity()){
							srckeys.push_back(it->str());
						}
						if(srckeys.size() >= srckeys.capacity()){
							s = sNOOP;
						}
					}break;
				  case sNOOP:{
						lyramilk::data::string flag = it->str();
						std::transform(flag.begin(),flag.end(),flag.begin(),::tolower);
						if(flag == "weights"){
							s = sWEIGTHS;
						}else if(flag == "aggregate"){
							s = sAGGREGATE;
						}else{
							TODO();
						}
					}break;
				  case sWEIGTHS:{
						if(srcweights.size() < srcweights.capacity()){
							srcweights.push_back(*it);
						}
						if(srcweights.size() >= srcweights.capacity()){
							s = sNOOP;
						}
					}break;
				  case sAGGREGATE:{
						lyramilk::data::string flag = it->str();
						std::transform(flag.begin(),flag.end(),flag.begin(),::tolower);
						if(flag == "sum"){
							aggregate = redis_store_sum;
						}else if(flag == "min"){
							aggregate = redis_store_min;
						}else if(flag == "max"){
							aggregate = redis_store_max;
						}else{
							TODO();
						}
					}break;
				  default:
					TODO();
				}
			}
		}
		for(std::size_t i=0;i<srckeys.size();++i){
			if(srcweights.size() < i + 1){
				srcweights.push_back(1.0f);
			}
		}


		std::map<std::string,destvalueinfo> destvalues;
		std::map<std::string,destvalueinfo>::iterator vit;


		for(std::size_t i=0;i<srckeys.size();++i){
			lyramilk::data::string key = srckeys[i];
			double weights = srcweights[i];
			leveldb::ReadOptions ropt;
			leveldb_iterator it(rh.ldb->NewIterator(ropt));

			redis_leveldb_key k(rh.dbprefix(-1));
			k.append_type(redis_leveldb_key::KT_ZSET);
			k.append_string(key.c_str(),key.size());
			k.append_string("s",1);
			std::string prefix = k;
			prefix.push_back(redis_leveldb_key::KEY_DOUBLE);
			if(it) for(it->Seek(prefix);it->Valid();it->Next()){
				leveldb::Slice datakey = it->key();
				if(!datakey.starts_with(prefix)) break;
				datakey.remove_prefix(prefix.size());
				double score = rh.bytes2double(datakey);
				datakey.remove_prefix(sizeof(double) + 1);

				leveldb::Slice tabkey(key.c_str(),key.size());
				{
					std::string sdatakey = datakey.ToString();
					vit = destvalues.find(sdatakey);
					if(vit == destvalues.end()){
						destvalues[sdatakey].score = score;
					}else{
						vit->second.score = aggregate(vit->second.score,score * weights);
					}
					destvalues[sdatakey].ref ++;
				}
			}
		}

		// redis_del
		leveldb::ReadOptions ropt;

		const unsigned char types[] = {redis_leveldb_key::KT_BASE,redis_leveldb_key::KT_STRING,redis_leveldb_key::KT_ZSET,redis_leveldb_key::KT_SET,redis_leveldb_key::KT_LIST,redis_leveldb_key::KT_HASH};
		for(unsigned int i=0;i<sizeof(types);++i){
			redis_leveldb_key k(rh.dbprefix(-1));
			k.append_type(types[i]);
			k.append_string(destkey.c_str(),destkey.size());
			std::string prefix = k;
			prefix.push_back(redis_leveldb_key::KEY_STR);

			leveldb_iterator it(rh.ldb->NewIterator(ropt));
			if(it) for(it->Seek(prefix);it->Valid();it->Next()){
				if(!it->key().starts_with(prefix)) break;
				batch.Delete(it->key());
			}
		}

		// zadd
		lyramilk::data::uint64 zcount = 0;
		for(vit = destvalues.begin();vit != destvalues.end();++vit){
			destvalueinfo& di = vit->second;
			if(di.ref != numkeys) continue;
			++zcount;
			std::string value = vit->first;
			std::string data_key = rh.encode_zset_data_key(destkey,lyramilk::data::str(value));
			std::string score_key = rh.encode_zset_score_key(destkey,di.score,lyramilk::data::str(value));
			batch.Put(data_key,rh.double2bytes(di.score));
			batch.Put(score_key,leveldb::Slice());
		}
		rh.set_size(batch,destkey,zcount);
	}

	static __attribute__ ((constructor)) void __init()
	{
		cavedb_redis_commands::instance()->define("zadd",redis_zadd);
		cavedb_redis_commands::instance()->define("zincrby",redis_zincrby);
		cavedb_redis_commands::instance()->define("zrem",redis_zrem);
		cavedb_redis_commands::instance()->define("zremrangebyrank",redis_zremrangebyrank);
		cavedb_redis_commands::instance()->define("zremrangebyscore",redis_zremrangebyscore);
		cavedb_redis_commands::instance()->define("zunionstore",redis_zunionstore);
		cavedb_redis_commands::instance()->define("zinterstore",redis_zinterstore);
	}


	bool database::zscan(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,zset_call_back cbk,void* userdata)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		leveldb::ReadOptions ropt;
		leveldb_iterator it(rh.ldb->NewIterator(ropt));

		redis_leveldb_key k(rh.dbprefix(dbid));
		k.append_type(redis_leveldb_key::KT_ZSET);
		k.append_string(key.c_str(),key.size());
		k.append_string("s",1);
		std::string prefix = k;
		prefix.push_back(redis_leveldb_key::KEY_DOUBLE);
		if(it) for(it->Seek(prefix);it->Valid();it->Next()){
			leveldb::Slice datakey = it->key();
			if(!datakey.starts_with(prefix)) break;
			datakey.remove_prefix(prefix.size());
			double score = rh.bytes2double(datakey);
			datakey.remove_prefix(sizeof(double) + 1);

			if(!cbk(key,score,datakey,userdata)) return false;
		}
		return true;
	}


	lyramilk::data::uint64 database::zcard(lyramilk::data::uint64 dbid,const lyramilk::data::string& key)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		return rh.get_size(key,dbid);
	}

	lyramilk::data::uint64 database::zcount(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,double min,double max)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		leveldb::ReadOptions ropt;
		leveldb_iterator it(rh.ldb->NewIterator(ropt));

		redis_leveldb_key k(rh.dbprefix(dbid));
		k.append_type(redis_leveldb_key::KT_ZSET);
		k.append_string(key.c_str(),key.size());
		k.append_string("s",1);
		std::string prefix = k;
		prefix.push_back(redis_leveldb_key::KEY_DOUBLE);
		k.append_double(min);
		std::string keybegin = k;

		lyramilk::data::uint64 ret = 0;

		if(it) for(it->Seek(keybegin);it->Valid();it->Next()){
			leveldb::Slice datakey = it->key();
			if(!datakey.starts_with(prefix)) break;
			datakey.remove_prefix(prefix.size());
			double score = rh.bytes2double(datakey);
			if(score >= max) break;
			++ret;
		}
		return ret;
	}

	bool database::zrangebyscore(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,double min,double max,zset_call_back cbk,void* userdata)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		leveldb::ReadOptions ropt;
		leveldb_iterator it(rh.ldb->NewIterator(ropt));

		redis_leveldb_key k(rh.dbprefix(dbid));
		k.append_type(redis_leveldb_key::KT_ZSET);
		k.append_string(key.c_str(),key.size());
		k.append_string("s",1);
		std::string prefix = k;
		prefix.push_back(redis_leveldb_key::KEY_DOUBLE);
		k.append_double(min);
		std::string keybegin = k;

		if(it) for(it->Seek(keybegin);it->Valid();it->Next()){
			leveldb::Slice datakey = it->key();
			if(!datakey.starts_with(prefix)) break;
			datakey.remove_prefix(prefix.size());
			double score = rh.bytes2double(datakey);
			if(score >= max) break;
			if(!cbk(key,score,datakey,userdata)) return false;
		}
		return true;
	}

	bool database::zrevrangebyscore(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,double min,double max,zset_call_back cbk,void* userdata)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		leveldb::ReadOptions ropt;
		leveldb_iterator it(rh.ldb->NewIterator(ropt));

		redis_leveldb_key k(rh.dbprefix(dbid));
		k.append_type(redis_leveldb_key::KT_ZSET);
		k.append_string(key.c_str(),key.size());
		k.append_string("s",1);
		std::string prefix = k;
		prefix.push_back(redis_leveldb_key::KEY_DOUBLE);
		k.append_double(max);

		std::string keybegin = k;

		if(it){
			it->Seek(keybegin);
			if(it->Valid()) it->Prev();
			for(;it->Valid();it->Prev()){
				leveldb::Slice datakey = it->key();
				if(!datakey.starts_with(prefix)) break;
				datakey.remove_prefix(prefix.size());
				double score = rh.bytes2double(datakey);
				if(score < min) break;
				if(!cbk(key,score,datakey,userdata)) return false;
			}
		}
		return true;
	}

	lyramilk::data::uint64 database::zrevrank(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& value)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		leveldb::ReadOptions ropt;
		leveldb_iterator it(rh.ldb->NewIterator(ropt));

		redis_leveldb_key k(rh.dbprefix(dbid));
		k.append_type(redis_leveldb_key::KT_ZSET);
		k.append_string(key.c_str(),key.size());
		k.append_string("s",1);
		std::string prefix = k;
		prefix.push_back(redis_leveldb_key::KEY_DOUBLE);

		lyramilk::data::uint64 ret = 0;

		if(it) for(it->Seek(prefix);it->Valid();it->Next()){
			leveldb::Slice datakey = it->key();
			if(!datakey.starts_with(prefix)) break;
			datakey.remove_prefix(prefix.size());
			if(value.size() == datakey.size() && value.compare(0,value.size(),datakey.data(),datakey.size()) == 0) return ret;
			++ret;
		}
		return -1;
	}

	lyramilk::data::uint64 database::zrank(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& value)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		leveldb::ReadOptions ropt;
		leveldb_iterator it(rh.ldb->NewIterator(ropt));

		redis_leveldb_key k(rh.dbprefix(dbid));
		k.append_type(redis_leveldb_key::KT_ZSET);
		k.append_string(key.c_str(),key.size());
		k.append_string("s",1);
		std::string prefix = k;
		prefix.push_back(redis_leveldb_key::KEY_DOUBLE);

		lyramilk::data::uint64 ret = 0;

		if(it) for(it->Seek(prefix);it->Valid();it->Next()){
			leveldb::Slice datakey = it->key();
			if(!datakey.starts_with(prefix)) break;
			datakey.remove_prefix(prefix.size());
			if(value.size() == datakey.size() && value.compare(0,value.size(),datakey.data(),datakey.size()) == 0) return ret;
			++ret;
		}
		return -1;
	}

	double database::zscore(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& value)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		leveldb::ReadOptions ropt;

		std::string data_key = rh.encode_zset_data_key(key,value);
		std::string soldscore;
		leveldb::Status ldbs = rh.ldb->Get(rh.ropt,data_key,&soldscore);
		if(ldbs.ok()){
			return rh.bytes2double(soldscore);
		}
		return 0.0/0.0;
	}

	bool database::zrange(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,lyramilk::data::int64 start,lyramilk::data::int64 stop,zset_call_back cbk,void* userdata)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		leveldb::ReadOptions ropt;
		leveldb_iterator it(rh.ldb->NewIterator(ropt));

		redis_leveldb_key k(rh.dbprefix(dbid));
		k.append_type(redis_leveldb_key::KT_ZSET);
		k.append_string(key.c_str(),key.size());
		k.append_string("s",1);
		std::string prefix = k;
		prefix.push_back(redis_leveldb_key::KEY_DOUBLE);
		lyramilk::data::int64 index =0;
		if(it) for(it->Seek(prefix);it->Valid();it->Next()){
			leveldb::Slice datakey = it->key();
			if(!datakey.starts_with(prefix)) break;
			if(index <= start) continue;
			if(index >= stop) break;
			++index;
			datakey.remove_prefix(prefix.size());
			double score = rh.bytes2double(datakey);
			if(!cbk(key,score,datakey,userdata)) return false;
		}
		return true;
	}

	bool database::zrevrange(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,lyramilk::data::int64 start,lyramilk::data::int64 stop,zset_call_back cbk,void* userdata)
	{
		redis_leveldb_handler& rh = *redis_cmd_args;
		leveldb::ReadOptions ropt;
		leveldb_iterator it(rh.ldb->NewIterator(ropt));

		redis_leveldb_key k(rh.dbprefix(dbid));
		k.append_type(redis_leveldb_key::KT_ZSET);
		k.append_string(key.c_str(),key.size());
		k.append_string("s",1);
		std::string prefix = k;
		std::string key_eof = prefix;
		key_eof.push_back(redis_leveldb_key::KEY_DOUBLE + 1);

		if(it){
			lyramilk::data::int64 index =0;
			it->Seek(key_eof);
			if(it->Valid()) it->Prev();
			for(;it->Valid();it->Prev()){
				leveldb::Slice datakey = it->key();
				if(!datakey.starts_with(prefix)) break;
				if(index <= start) continue;
				if(index >= stop) break;
				++index;
				datakey.remove_prefix(prefix.size());
				double score = rh.bytes2double(datakey);
				if(!cbk(key,score,datakey,userdata)) return false;
			}
		}
		return true;
	}

}}
