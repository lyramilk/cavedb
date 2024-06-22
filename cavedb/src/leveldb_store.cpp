#include "leveldb_store.h"
#include "cavedb_key.h"
#include "util.h"

#include <libmilk/log.h>
#include <libmilk/testing.h>
#include <libmilk/dict.h>
#include <libmilk/exception.h>
#include <libmilk/hash.h>
#include <libmilk/stringutil.h>

#include <leveldb/db.h>
#include <leveldb/filter_policy.h>
#include <leveldb/cache.h>
#include <leveldb/write_batch.h>
#include <leveldb/comparator.h>
#include <leveldb/slice.h>

#include <unistd.h>


/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{

	leveldb::ReadOptions ropt;
	lyramilk::log::logss static log(lyramilk::klog,"lyramilk.cave.leveldb_store");

	speed_counter::speed_counter()
	{
		speed_count = 0;
		speed_tm = 0;
		speed = 0;

	}

	speed_counter::~speed_counter()
	{}

	lyramilk::data::uint64 speed_counter::operator ++()
	{
		__sync_fetch_and_add(&speed_count,1);

		time_t tm_now = time(nullptr);
		if(tm_now != speed_tm){
			speed_tm = tm_now;
			speed = speed_count;
			speed_count = 0;
		}
		return speed;
	}

	speed_counter::operator lyramilk::data::uint64()
	{
		time_t tm_now = time(nullptr);
		if(tm_now != speed_tm){
			speed_tm = tm_now;
			speed = speed_count;
			speed_count = 0;
		}
		return speed;
	}


	leveldb::Status inline ldb_put(leveldb::DB* db,binlog* blog,const cavedb_key& rpack,const lyramilk::data::string& value)
	{
		if(blog){
			if(rpack.type == cavedb_key_type::s_hash){
				blog->hset(rpack.key.ToString(),rpack.hash.field.ToString(),value);
			}else if(rpack.type == cavedb_key_type::s_set){
				blog->sadd(rpack.key.ToString(),rpack.set.member.ToString());
			}else{
				throw lyramilk::exception(D("cavedb_key未处理的类型%d",rpack.type));
			}
		}


		static leveldb::WriteOptions wopt;

		std::string lkey = cavedb_key::stringify(rpack);
		std::string lvalue = lyramilk::data::str(value);

		return db->Put(wopt,lkey,lvalue);
	}

	leveldb::Status inline ldb_del(leveldb::DB* db,binlog* blog,const cavedb_key& rpack)
	{
		if(blog){
			if(rpack.type == cavedb_key_type::s_hash){
				blog->hdel(rpack.key.ToString(),rpack.hash.field.ToString());
			}else if(rpack.type == cavedb_key_type::s_set){
				blog->srem(rpack.key.ToString(),rpack.set.member.ToString());
			}else{
				throw lyramilk::exception(D("cavedb_key未处理的类型%d",rpack.type));
			}
		}

		static leveldb::WriteOptions wopt;
		std::string lkey = cavedb_key::stringify(rpack);
		return db->Delete(wopt,lkey);
	}



	leveldb::Status inline ldb_put_with_seq(leveldb::DB* db,binlog* blog,lyramilk::data::uint64 seq,const cavedb_key& rpack,const lyramilk::data::string& value)
	{
		if(blog){
			if(rpack.type == cavedb_key_type::s_hash){
				blog->hset_with_seq(seq,rpack.key.ToString(),rpack.hash.field.ToString(),value);
			}else if(rpack.type == cavedb_key_type::s_set){
				blog->sadd_with_seq(seq,rpack.key.ToString(),rpack.set.member.ToString());
			}else if(rpack.type == cavedb_key_type::s_zset){
				blog->zadd_with_seq(seq,rpack.key.ToString(),rpack.zset.score,rpack.zset.member.ToString());
			}else if(rpack.type == cavedb_key_type::s_zset_m2s){
			}else{
				throw lyramilk::exception(D("cavedb_key未处理的类型%d",rpack.type));
			}
		}


		static leveldb::WriteOptions wopt;

		std::string lkey = cavedb_key::stringify(rpack);
		std::string lvalue = lyramilk::data::str(value);

		return db->Put(wopt,lkey,lvalue);
	}

	leveldb::Status inline ldb_del_with_seq(leveldb::DB* db,binlog* blog,lyramilk::data::uint64 seq,const cavedb_key& rpack)
	{
		if(blog){
			if(rpack.type == cavedb_key_type::s_hash){
				blog->hdel_with_seq(seq,rpack.key.ToString(),rpack.hash.field.ToString());
			}else if(rpack.type == cavedb_key_type::s_set){
				blog->srem_with_seq(seq,rpack.key.ToString(),rpack.set.member.ToString());
			}else if(rpack.type == cavedb_key_type::s_zset){
				blog->zrem_with_seq(seq,rpack.key.ToString(),rpack.zset.member.ToString());
			}else if(rpack.type == cavedb_key_type::s_zset_m2s){
			}else{
				throw lyramilk::exception(D("cavedb_key未处理的类型%d",rpack.type));
			}
		}

		static leveldb::WriteOptions wopt;
		std::string lkey = cavedb_key::stringify(rpack);
		return db->Delete(wopt,lkey);
	}

	//leveldb::DB* ldb;

	cmdstatus leveldb_store::on_hgetall(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::data::stringdict sd;
		lyramilk::data::string key = args[1].str();

		cmdstatus r = hgetall(key,&sd);
		if(r == cs_data){
			*ret = sd;
		}
		return r;
	}

	cmdstatus leveldb_store::on_hget(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::data::string val;
		lyramilk::data::string key = args[1].str();
		lyramilk::data::string field = args[2].str();

		cmdstatus r = hget(key,field,&val);
		if(r == cs_data){
			*ret = val;
		}
		return r;
	}

	cmdstatus leveldb_store::on_hmget(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		ret->type(lyramilk::data::var::t_array);
		lyramilk::data::array& ar = *ret;

		lyramilk::data::string key = args[1].str();

		for(lyramilk::data::array::const_iterator it = args.begin() + 2;it!=args.end();++it){
			lyramilk::data::string val;
			cmdstatus r = hget(key,it->str(),&val);
			if(r == cmdstatus::cs_data){
				ar.push_back(val);
			}else{
				ar.push_back(lyramilk::data::var::nil);
			}
		}
		return cmdstatus::cs_data;



	}

	cmdstatus leveldb_store::on_hexist(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::string key = args[1].str();
		lyramilk::data::string field = args[2].str();

		cavedb_key pk;
		pk.type = cavedb_key::s_hash;

		pk.key = key;
		pk.hash.field.assign(field.data(),field.size());

		std::string lkey = cavedb_key::stringify(pk);

		std::string result;
		leveldb::Status ldbs = ldb->Get(ropt,lkey,&result);

		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,__FUNCTION__) << D("命令 hexist %.*s,%.*s 耗时%.3f",key.size(),key.c_str(),field.size(),field.c_str(),double(nsec) / 1000000) << std::endl;
		}

		if(ldbs.ok()){
			*ret = 1;
			return cmdstatus::cs_data;
		}
		if(ldbs.IsNotFound()){
			*ret = 0;
			return cmdstatus::cs_data;
		}

		log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
		*ret = ldbs.ToString();
		return cmdstatus::cs_error;
	}

	cmdstatus leveldb_store::on_hset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		lyramilk::data::string key = args[1].str();

		cavedb_key pk;
		pk.type = cavedb_key::s_hash;
		pk.key = key;

		lyramilk::data::uint64 c = args.size();
		lyramilk::data::uint64 r = 0;
		if(args.size() % 2 != 0){
			--c;
		}

		leveldb::Status ldbs;

		for(lyramilk::data::uint64 i=2;i<c;i+=2){
			++r;
			lyramilk::data::string field = args[i].str();
			pk.hash.field.assign(field.data(),field.size());

			ldbs = ldb_put(ldb,blog,pk,args[i + 1].str());
			if(!ldbs.ok()) break;
		}

		long long nsec = nd.diff();
		if(nsec > 200000000){

			lyramilk::data::string str;

			for(lyramilk::data::uint64 i=2;i<c;i+=2){
				str.append(" ");
				str.append(lyramilk::data::str(args[i].str()));
				str.append("=");
				str.append(lyramilk::data::str(args[i + 1].str()));
			}

			log(lyramilk::log::warning,__FUNCTION__) << D("命令 hset %.*s%.*s 耗时%.3f",key.size(),key.c_str(),str.size(),str.c_str(),double(nsec) / 1000000) << std::endl;
		}


		if(ldbs.ok()){
			*ret = r;
			return cmdstatus::cs_ok;
		}

		log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
		*ret = ldbs.ToString();
		return cmdstatus::cs_error;
	}

	cmdstatus leveldb_store::on_hdel(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::string key = args[1].str();

		cavedb_key pk;
		pk.type = cavedb_key::s_hash;
		pk.key = key;

		lyramilk::data::uint64 c = args.size();
		lyramilk::data::uint64 r = 0;

		leveldb::Status ldbs;

		for(lyramilk::data::uint64 i=2;i<c;++i){
			++r;
			lyramilk::data::string field = args[i].str();
			pk.hash.field.assign(field.data(),field.size());

			ldbs = ldb_del(ldb,blog,pk);
			if(!ldbs.ok()) break;
		}

		long long nsec = nd.diff();
		if(nsec > 200000000){
			lyramilk::data::string str;
			for(lyramilk::data::uint64 i=2;i<c;++i){
				str.append(" ");
				str.append(lyramilk::data::str(args[i].str()));
			}

			log(lyramilk::log::warning,__FUNCTION__) << D("命令 hdel %.*s%.*s 耗时%.3f",key.size(),key.c_str(),str.size(),str.c_str(),double(nsec) / 1000000) << std::endl;
		}


		if(ldbs.ok()){
			*ret = r;
			return cmdstatus::cs_ok;
		}

		log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
		*ret = ldbs.ToString();
		return cmdstatus::cs_error;
	}

	cmdstatus leveldb_store::on_hscan(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::data::string key = args[1].str();
		lyramilk::data::uint64 current_hash = args[2].conv(0ull);
		lyramilk::data::string cursor;

		if(current_hash != 0){
			std::map<lyramilk::data::uint64,lyramilk::data::string>::const_iterator it = sen->rainbow_table.find(current_hash);
			if(it!=sen->rainbow_table.end()){
				cursor = it->second;
			}else{
				*ret = "ERR invalid cursor";
				return cmdstatus::cs_error;
			}
		}else{
			sen->rainbow_table.clear();
		}

		lyramilk::data::strings results;
		lyramilk::data::string next_cursor;
		leveldb_store::hscan(key,cursor,20,&next_cursor,&results);

		lyramilk::data::uint64 nexthash = 0;
		if(next_cursor.empty()){
			nexthash = 0;
		}else{
			nexthash = lyramilk::cryptology::hash64::fnv(next_cursor.data(),next_cursor.size());
			nexthash >>= 1;
			
			if(sen->rainbow_table.size() > 100) sen->rainbow_table.clear();
			sen->rainbow_table[nexthash] = next_cursor;
		}

		lyramilk::data::array resultdata;
		for(lyramilk::data::strings::iterator it = results.begin();it!=results.end();++it){
			resultdata.emplace_back(*it);
		}

		ret->type(lyramilk::data::var::t_array);
		lyramilk::data::array& ar = *ret;
		ar.resize(2);
		ar[0] = nexthash;
		lyramilk::data::array& inner_ar = ar[1].type(lyramilk::data::var::t_array);
		inner_ar.swap(resultdata);
		return cmdstatus::cs_data;
	}

	cmdstatus leveldb_store::on_sadd(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::string key = args[1].str();
		lyramilk::data::string value = args[2].str();

		cavedb_key pk;
		pk.type = cavedb_key::s_set;
		pk.key = key;

		lyramilk::data::uint64 c = args.size();
		lyramilk::data::uint64 r = 0;

		leveldb::Status ldbs;

		for(lyramilk::data::uint64 i=2;i<c;++i){
			++r;
			lyramilk::data::string value = args[i].str();
			pk.set.member.assign(value.data(),value.size());

			ldbs = ldb_put(ldb,blog,pk,"");
			if(!ldbs.ok()) break;

		}

		long long nsec = nd.diff();
		if(nsec > 200000000){
			lyramilk::data::string str;

			for(lyramilk::data::uint64 i=2;i<c;i+=2){
				str.append(" ");
				str.append(lyramilk::data::str(args[i].str()));
			}

			log(lyramilk::log::warning,__FUNCTION__) << D("命令 sadd %.*s%.*s 耗时%.3f",key.size(),key.c_str(),str.size(),str.c_str(),double(nsec) / 1000000) << std::endl;
		}

		if(ldbs.ok()){
			*ret = 1;
			return cmdstatus::cs_data;
		}

		if(ldbs.IsNotFound()){
			*ret = 0;
			return cmdstatus::cs_data;
		}

		log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
		*ret = ldbs.ToString();
		return cmdstatus::cs_error;
	}

	cmdstatus leveldb_store::on_srem(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::string key = args[1].str();

		cavedb_key pk;
		pk.type = cavedb_key::s_set;
		pk.key = key;

		lyramilk::data::uint64 c = args.size();
		lyramilk::data::uint64 r = 0;

		leveldb::Status ldbs;

		for(lyramilk::data::uint64 i=2;i<c;++i){
			++r;
			lyramilk::data::string value = args[i].str();
			pk.set.member.assign(value.data(),value.size());

			ldbs = ldb_del(ldb,blog,pk);
			if(!ldbs.ok()) break;
		}

		long long nsec = nd.diff();
		if(nsec > 200000000){
			lyramilk::data::string str;

			for(lyramilk::data::uint64 i=2;i<c;i+=2){
				str.append(" ");
				str.append(lyramilk::data::str(args[i].str()));
			}

			log(lyramilk::log::warning,__FUNCTION__) << D("命令 srem %.*s%.*s 耗时%.3f",key.size(),key.c_str(),str.size(),str.c_str(),double(nsec) / 1000000) << std::endl;
		}

		if(ldbs.ok()){
			*ret = 1;
			return cmdstatus::cs_data;
		}

		if(ldbs.IsNotFound()){
			*ret = 0;
			return cmdstatus::cs_data;
		}

		log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
		*ret = ldbs.ToString();
		return cmdstatus::cs_error;
	}

	cmdstatus leveldb_store::on_smembers(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::data::strings sd;
		lyramilk::data::string key = args[1].str();

		cmdstatus r = smembers(key,&sd);
		if(r == cs_data){
			ret->type(lyramilk::data::var::t_array);
			lyramilk::data::array& ar = *ret;

			ar.reserve(sd.size());
			for(long unsigned int i=0;i<sd.size();++i){
				ar.emplace_back(sd[i]);
			}
		}
		return r;
	}

	cmdstatus leveldb_store::on_sscan(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::data::string key = args[1].str();
		lyramilk::data::uint64 current_hash = args[2].conv(0ull);
		lyramilk::data::string cursor;

		if(current_hash != 0){
			std::map<lyramilk::data::uint64,lyramilk::data::string>::const_iterator it = sen->rainbow_table.find(current_hash);
			if(it!=sen->rainbow_table.end()){
				cursor = it->second;
			}else{
				*ret = "ERR invalid cursor";
				return cmdstatus::cs_error;
			}
		}else{
			sen->rainbow_table.clear();
		}

		lyramilk::data::strings results;
		lyramilk::data::string next_cursor;
		leveldb_store::sscan(key,cursor,20,&next_cursor,&results);

		lyramilk::data::uint64 nexthash = 0;
		if(next_cursor.empty()){
			nexthash = 0;
		}else{
			
			nexthash = lyramilk::cryptology::hash64::fnv(next_cursor.data(),next_cursor.size());
			nexthash >>= 1;
			
			if(sen->rainbow_table.size() > 100) sen->rainbow_table.clear();
			sen->rainbow_table[nexthash] = next_cursor;
		}

		lyramilk::data::array resultdata;
		for(lyramilk::data::strings::iterator it = results.begin();it!=results.end();++it){
			resultdata.emplace_back(*it);
		}

		ret->type(lyramilk::data::var::t_array);
		lyramilk::data::array& ar = *ret;
		ar.resize(2);
		ar[0] = nexthash;
		lyramilk::data::array& inner_ar = ar[1].type(lyramilk::data::var::t_array);
		inner_ar.swap(resultdata);
		return cmdstatus::cs_data;
	}

	cmdstatus leveldb_store::on_del(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::string key = args[1].str();

		long ditems = 0;
		leveldb::Status ldbs;

		std::string start;
		std::string end;

		lyramilk::data::string result_cursor;
		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			}else{

				lyramilk::data::string keep_prefix = cavedb_key::make_key_prefix(key);
				start = keep_prefix;

				it->Seek(keep_prefix);
				if(it->Valid()){
					for(;it->Valid();it->Next()){
						if(!it->key().starts_with(keep_prefix)){
							end = it->key().ToString();
							break;
						}

						cavedb_key spack;
						if(!cavedb_key::parse(it->key(),&spack)){
							log(lyramilk::log::error,__FUNCTION__) << D("%s错误：遇到无法解析的内容\n",__FUNCTION__) << std::endl;
							return cmdstatus::cs_error;
						}

						ldbs = ldb_del(ldb,blog,spack);
						if(!ldbs.ok()) break;

						++ditems;
					}
				}

				if (it) delete it;
			}
		}

		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return cmdstatus::cs_error;
		}else{
			if(ditems > 1000){
				leveldb::Slice a(start);
				leveldb::Slice b(end);
				ldb->CompactRange(&a,&b);
			}
		}
		return cmdstatus::cs_ok;
	}


	cmdstatus leveldb_store::on_cave_sync(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		// cavedb_sync [key] [seq] [count]
		lyramilk::data::string key = args[1].str();
		lyramilk::data::uint64 seq = args[2].conv(0);
		lyramilk::data::int64 count = args[3].conv(30000);
		bool withseq = false;
		if(args.size() > 4){
			for(int i=4;i<args.size();++i){
				lyramilk::data::string str = lyramilk::data::lower_case(args[i].str());
				if(str == "withseq") {
					withseq = true;
				}
			}
		}

		ret->type(lyramilk::data::var::t_array);
		lyramilk::data::array& ar = *ret;

		ar.resize(3);
		ar[2].type(lyramilk::data::var::t_array);
		lyramilk::data::array& ardata = ar[2];

		lyramilk::data::string nextkey;
		lyramilk::data::uint64 nextseq = 0;

		// 扫描全数据
		if( (!key.empty()) || seq == 0){
			if(seq == 0){
				if(blog == nullptr){
					nextseq = 1;
				}else{
					nextseq = blog->find_max() + 1;
				}
			}else{
				nextseq = seq;
			}

			//如果key不为空，或key为空且seq为0，且扫描本地数据。
			leveldb::Iterator* it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
				*ret = "ERR create iterator fail";
				return cmdstatus::cs_error;
			}else{

				if(seq == 0){
					//从0开始启动
					lyramilk::data::var v;
					ardata.push_back(v);
					ardata.back().type(lyramilk::data::var::t_array);

					lyramilk::data::array& ar = ardata.back();
					ar.push_back("sync_start");
				}

				it->Seek(key);
				for(lyramilk::data::int64 i =0;it->Valid() && i < count;it->Next(),++i){
					cavedb_key spack;
					if(cavedb_key::parse(it->key(),&spack)){
						if(spack.type == cavedb_key::s_hash){
							lyramilk::data::var v;
							ardata.push_back(v);
							ardata.back().type(lyramilk::data::var::t_array);

							lyramilk::data::array& ar = ardata.back();
							ar.push_back(withseq?"binlog_hset":"hset");
							ar.push_back(lyramilk::data::str(spack.key.ToString()));
							ar.push_back(lyramilk::data::str(spack.hash.field.ToString()));
							ar.push_back(lyramilk::data::str(it->value().ToString()));
							if(withseq) ar.push_back(0);
						}else if(spack.type == cavedb_key::s_set){
							lyramilk::data::var v;
							ardata.push_back(v);
							ardata.back().type(lyramilk::data::var::t_array);

							lyramilk::data::array& ar = ardata.back();
							ar.push_back(withseq?"binlog_sadd":"sadd");
							ar.push_back(lyramilk::data::str(spack.key.ToString()));
							ar.push_back(lyramilk::data::str(spack.set.member.ToString()));
							if(withseq) ar.push_back(0);
						}else if(spack.type == cavedb_key::s_zset){
							lyramilk::data::var v;
							ardata.push_back(v);
							ardata.back().type(lyramilk::data::var::t_array);

							lyramilk::data::array& ar = ardata.back();
							ar.push_back(withseq?"binlog_zadd":"zadd");
							ar.push_back(lyramilk::data::str(spack.key.ToString()));
							ar.push_back(spack.zset.score);
							ar.push_back(lyramilk::data::str(spack.zset.member.ToString()));
							if(withseq) ar.push_back(0);
						}
					}
				}

				if(it->Valid()){
					nextkey = lyramilk::data::str(it->key().ToString());
				}else{
					nextkey.clear();
					// 遍历本地数据完成
					lyramilk::data::var v;
					ardata.push_back(v);
					ardata.back().type(lyramilk::data::var::t_array);

					lyramilk::data::array& ar = ardata.back();
					ar.push_back("sync_continue");
				}

				if (it) delete it;
			}
			ar[0] = nextkey;
			ar[1] = nextseq;
			return cmdstatus::cs_data; 
		}

		// 读取binlog
		if(blog == nullptr){
			*ret = "ERR binlog is not configured";
			return cmdstatus::cs_error;
		}
		blog->read(seq,count,&ardata,&nextseq,withseq);
		ar[0] = nextkey;
		ar[1] = nextseq;
		return cmdstatus::cs_data;
	}
	
	
	cmdstatus leveldb_store::on_scan(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		// scan [cursor]
		lyramilk::data::uint64 current_hash = args[1].conv(0ull);
		lyramilk::data::string cursor;

		if(current_hash != 0){
			std::map<lyramilk::data::uint64,lyramilk::data::string>::const_iterator it = sen->rainbow_table.find(current_hash);
			if(it!=sen->rainbow_table.end()){
				cursor = it->second;
			}else{
				*ret = "ERR invalid cursor";
				return cmdstatus::cs_error;
			}
		}else{
			sen->rainbow_table.clear();
		}

		lyramilk::data::strings results;
		lyramilk::data::string next_cursor;
		leveldb_store::scan(cursor,20,&next_cursor,&results);

		lyramilk::data::uint64 nexthash = 0;
		if(next_cursor.empty()){
			nexthash = 0;
		}else{
			nexthash = lyramilk::cryptology::hash64::fnv(next_cursor.data(),next_cursor.size());
			nexthash >>= 1;
			
			if(sen->rainbow_table.size() > 100) sen->rainbow_table.clear();
			sen->rainbow_table[nexthash] = next_cursor;
		}

		lyramilk::data::array resultdata;
		for(lyramilk::data::strings::iterator it = results.begin();it!=results.end();++it){
			resultdata.emplace_back(*it);
		}

		ret->type(lyramilk::data::var::t_array);
		lyramilk::data::array& ar = *ret;
		ar.resize(2);
		ar[0] = nexthash;
		lyramilk::data::array& inner_ar = ar[1].type(lyramilk::data::var::t_array);
		inner_ar.swap(resultdata);
		return cmdstatus::cs_data;
	}
	
	
	cmdstatus leveldb_store::on_type(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		// type [key]
		lyramilk::data::string key = args[1].str();
		cavedb_key_type keytype = cavedb_key_type::s_none;
		cmdstatus r = type(key,&keytype);
		if(r == cmdstatus::cs_data){
			if(keytype == cavedb_key::s_string){
				*ret = "string";
				return cmdstatus::cs_data;
			}else if(keytype == cavedb_key::s_hash){
				*ret = "hash";
				return cmdstatus::cs_data;
			}else if(keytype == cavedb_key::s_list){
				*ret = "list";
				return cmdstatus::cs_data;
			}else if(keytype == cavedb_key::s_set){
				*ret = "set";
				return cmdstatus::cs_data;
			}else if(keytype == cavedb_key::s_zset){
				*ret = "zset";
				return cmdstatus::cs_data;
			}else{
				*ret = "none";
				return cmdstatus::cs_data;
			}
		}
		return r;
	}

	cmdstatus leveldb_store::on_binlog_hset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::string key = args[1].str();
		lyramilk::data::string field = args[2].str();
		lyramilk::data::string value = args[3].str();
		lyramilk::data::uint64 seq = args[4].conv(0);

		cavedb_key pk;
		pk.type = cavedb_key::s_hash;
		pk.key = key;
		pk.hash.field.assign(field.data(),field.size());

		leveldb::Status ldbs = ldb_put_with_seq(ldb,blog,seq,pk,value);

		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,__FUNCTION__) << D("命令 hset %.*s %.*s %.*s 耗时%.3f",key.size(),key.c_str(),field.size(),field.c_str(),value.size(),value.c_str(),double(nsec) / 1000000) << std::endl;
		}


		if(ldbs.ok()){
			*ret = 1;
			return cmdstatus::cs_ok;
		}

		log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
		*ret = ldbs.ToString();
		return cmdstatus::cs_error;
	}

	cmdstatus leveldb_store::on_binlog_hdel(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::string key = args[1].str();
		lyramilk::data::string field = args[2].str();
		lyramilk::data::uint64 seq = args[3].conv(0);

		cavedb_key pk;
		pk.type = cavedb_key::s_hash;
		pk.key = key;
		pk.hash.field.assign(field.data(),field.size());

		leveldb::Status ldbs = ldb_del_with_seq(ldb,blog,seq,pk);

		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,__FUNCTION__) << D("命令 hdel %.*s %.*s 耗时%.3f",key.size(),key.c_str(),field.size(),field.c_str(),double(nsec) / 1000000) << std::endl;
		}


		if(ldbs.ok()){
			*ret = 1;
			return cmdstatus::cs_ok;
		}

		log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
		*ret = ldbs.ToString();
		return cmdstatus::cs_error;
	}

	cmdstatus leveldb_store::on_binlog_sadd(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::string key = args[1].str();
		lyramilk::data::string value = args[2].str();
		lyramilk::data::uint64 seq = args[3].conv(0);

		cavedb_key pk;
		pk.type = cavedb_key::s_set;
		pk.key = key;
		pk.set.member.assign(value.data(),value.size());

		leveldb::Status ldbs = ldb_put_with_seq(ldb,blog,seq,pk,"");

		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,__FUNCTION__) << D("命令 sadd %.*s %.*s 耗时%.3f",key.size(),key.c_str(),value.size(),value.c_str(),double(nsec) / 1000000) << std::endl;
		}

		if(ldbs.ok()){
			*ret = 1;
			return cmdstatus::cs_data;
		}

		if(ldbs.IsNotFound()){
			*ret = 0;
			return cmdstatus::cs_data;
		}

		log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
		*ret = ldbs.ToString();
		return cmdstatus::cs_error;
	}

	cmdstatus leveldb_store::on_binlog_srem(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::string key = args[1].str();
		lyramilk::data::string value = args[2].str();
		lyramilk::data::uint64 seq = args[3].conv(0);

		cavedb_key pk;
		pk.type = cavedb_key::s_set;
		pk.key = key;
		pk.set.member.assign(value.data(),value.size());

		leveldb::Status ldbs = ldb_del_with_seq(ldb,blog,seq,pk);

		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,__FUNCTION__) << D("命令 srem %.*s %.*s 耗时%.3f",key.size(),key.c_str(),value.size(),value.c_str(),double(nsec) / 1000000) << std::endl;
		}

		if(ldbs.ok()){
			*ret = 1;
			return cmdstatus::cs_data;
		}

		if(ldbs.IsNotFound()){
			*ret = 0;
			return cmdstatus::cs_data;
		}

		log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
		*ret = ldbs.ToString();
		return cmdstatus::cs_error;
	}

	cmdstatus leveldb_store::on_binlog_zadd(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::string key = args[1].str();
		double score = args[2].conv(0.0f);
		lyramilk::data::string value = args[3].str();
		lyramilk::data::uint64 seq = args[4].conv(0);

		leveldb::Status ldbs;
		{
			// s_zset_m2s
			cavedb_key pk;
			pk.type = cavedb_key::s_zset_m2s;
			pk.key = key;
			pk.zset.member.assign(value.data(),value.size());
			pk.zset.score = score;

			ldbs = ldb_put_with_seq(ldb,blog,seq,pk,"");
			if(!ldbs.ok()){
				log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
				*ret = ldbs.ToString();
				return cmdstatus::cs_error;
			}
		}

		{
			// s_zset
			cavedb_key pk;
			pk.type = cavedb_key::s_zset;
			pk.key = key;
			pk.zset.member.assign(value.data(),value.size());

			ldbs = ldb_put_with_seq(ldb,blog,seq,pk,"");
		}

		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,__FUNCTION__) << D("命令 sadd %.*s %f %.*s 耗时%.3f",key.size(),key.c_str(),score,value.size(),value.c_str(),double(nsec) / 1000000) << std::endl;
		}

		if(ldbs.ok()){
			*ret = 1;
			return cmdstatus::cs_data;
		}

		if(ldbs.IsNotFound()){
			*ret = 0;
			return cmdstatus::cs_data;
		}

		log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
		*ret = ldbs.ToString();
		return cmdstatus::cs_error;
	}

	cmdstatus leveldb_store::on_binlog_zrem(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::string key = args[1].str();
		lyramilk::data::string value = args[2].str();
		lyramilk::data::uint64 seq = args[3].conv(0);


		leveldb::Status ldbs;
		{
			cavedb_key pk;
			pk.type = cavedb_key::s_zset_m2s;
			pk.key = key;
			pk.zset.member.assign(value.data(),value.size());

			// 从s_zset_m2s读score
			std::string lkey = cavedb_key::stringify(pk);
			std::string data;
			ldbs = ldb->Get(ropt,lkey,&data);
			
			if(!ldbs.ok()){
				log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
				*ret = ldbs.ToString();
				return cmdstatus::cs_error;
			}
			
			if(data.size() == sizeof(double)){
				double score = 0.0f;
				data.copy((char*)&score,sizeof(double));
				pk.zset.score = score;
			}

			// 删除 s_zset_m2s
			ldbs = ldb_del_with_seq(ldb,blog,seq,pk);
			if(!ldbs.ok()){
				log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
				*ret = ldbs.ToString();
				return cmdstatus::cs_error;
			}
			
			// 删除 s_zset
			pk.type = cavedb_key::s_zset;
			ldbs = ldb_del_with_seq(ldb,blog,seq,pk);
		}


		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,__FUNCTION__) << D("命令 srem %.*s %.*s 耗时%.3f",key.size(),key.c_str(),value.size(),value.c_str(),double(nsec) / 1000000) << std::endl;
		}

		if(ldbs.ok()){
			*ret = 1;
			return cmdstatus::cs_data;
		}

		if(ldbs.IsNotFound()){
			*ret = 0;
			return cmdstatus::cs_data;
		}

		log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
		*ret = ldbs.ToString();
		return cmdstatus::cs_error;
	}


	leveldb_store::leveldb_store()
	{
		ldb = nullptr;
		blog = nullptr;
		regist("hgetall",&command_method_2_function<leveldb_store,&leveldb_store::on_hgetall>,2,command_sepc::readonly|command_sepc::noscript,1,1,1);
		regist("hget",&command_method_2_function<leveldb_store,&leveldb_store::on_hget>,3,command_sepc::readonly|command_sepc::fast|command_sepc::noscript,1,1,1);
		regist("hmget",&command_method_2_function<leveldb_store,&leveldb_store::on_hmget>,-3,command_sepc::readonly|command_sepc::fast|command_sepc::noscript,1,1,1);
		regist("hexist",&command_method_2_function<leveldb_store,&leveldb_store::on_hexist>,3,command_sepc::readonly|command_sepc::fast|command_sepc::noscript,1,1,1);
		regist("hset",&command_method_2_function<leveldb_store,&leveldb_store::on_hset>,-4,command_sepc::write|command_sepc::fast|command_sepc::noscript,1,1,1);
		regist("hmset",&command_method_2_function<leveldb_store,&leveldb_store::on_hset>,-4,command_sepc::write|command_sepc::fast|command_sepc::noscript,1,1,1);
		regist("hdel",&command_method_2_function<leveldb_store,&leveldb_store::on_hdel>,-3,command_sepc::write|command_sepc::fast|command_sepc::noscript,1,1,1);
		regist("hscan",&command_method_2_function<leveldb_store,&leveldb_store::on_hscan>,3,command_sepc::readonly|command_sepc::noscript,1,1,1);

		regist("del",&command_method_2_function<leveldb_store,&leveldb_store::on_del>,2,command_sepc::write|command_sepc::noscript,1,1,1);

		regist("sadd",&command_method_2_function<leveldb_store,&leveldb_store::on_sadd>,-3,command_sepc::write|command_sepc::fast|command_sepc::noscript,1,1,1);
		regist("srem",&command_method_2_function<leveldb_store,&leveldb_store::on_srem>,-3,command_sepc::write|command_sepc::fast|command_sepc::noscript,1,1,1);
		regist("smembers",&command_method_2_function<leveldb_store,&leveldb_store::on_smembers>,2,command_sepc::readonly|command_sepc::noscript,1,1,1);
		regist("sscan",&command_method_2_function<leveldb_store,&leveldb_store::on_sscan>,3,command_sepc::readonly|command_sepc::noscript,1,1,1);

		regist("cave_sync",command_method_2_function<leveldb_store,&leveldb_store::on_cave_sync>,-4,command_sepc::skip_monitor|command_sepc::fast|command_sepc::noscript|command_sepc::readonly,0,0,0);
		regist("scan",command_method_2_function<leveldb_store,&leveldb_store::on_scan>,2,command_sepc::skip_monitor|command_sepc::noscript|command_sepc::readonly,0,0,0);
		regist("type",command_method_2_function<leveldb_store,&leveldb_store::on_type>,2,command_sepc::skip_monitor|command_sepc::fast|command_sepc::noscript|command_sepc::readonly,1,1,1);

		regist("binlog_hset",command_method_2_function<leveldb_store,&leveldb_store::on_binlog_hset>,5,command_sepc::skip_monitor|command_sepc::fast|command_sepc::noscript,1,1,1);
		regist("binlog_hdel",command_method_2_function<leveldb_store,&leveldb_store::on_binlog_hdel>,4,command_sepc::skip_monitor|command_sepc::fast|command_sepc::noscript,1,1,1);
		regist("binlog_sadd",command_method_2_function<leveldb_store,&leveldb_store::on_binlog_sadd>,4,command_sepc::skip_monitor|command_sepc::fast|command_sepc::noscript,1,1,1);
		regist("binlog_srem",command_method_2_function<leveldb_store,&leveldb_store::on_binlog_srem>,4,command_sepc::skip_monitor|command_sepc::fast|command_sepc::noscript,1,1,1);
		regist("binlog_zadd",command_method_2_function<leveldb_store,&leveldb_store::on_binlog_zadd>,5,command_sepc::skip_monitor|command_sepc::fast|command_sepc::noscript,1,1,1);
		regist("binlog_zrem",command_method_2_function<leveldb_store,&leveldb_store::on_binlog_zrem>,4,command_sepc::skip_monitor|command_sepc::fast|command_sepc::noscript,1,1,1);
	}

	leveldb_store::~leveldb_store()
	{
		
	}

	void* leveldb_store::thread_auto_compact(leveldb::DB* ldb)
	{
		lyramilk::data::string cursor;
		lyramilk::data::string lastcur;
		leveldb::WriteOptions wopt;

		std::string lkey;
		{
			cavedb_key pk;
			pk.type = cavedb_key::s_native;
			pk.key = ".sync.last_auto_compact";
			lkey = cavedb_key::stringify(pk);
		}

		{
			leveldb::Status ldbs = ldb->Get(ropt,lkey,&cursor);

			if(ldbs.ok()){
				log(lyramilk::log::trace,__FUNCTION__) << D("自动整理进度:\"%s\"",encode_for_print(cursor).c_str()) << std::endl;
			}else if(ldbs.IsNotFound()){
				log(lyramilk::log::debug,__FUNCTION__) << D("未找到自动整理进度") << std::endl;
			}else{
				log(lyramilk::log::warning,__FUNCTION__) << D("自动整理进度出错:%s",ldbs.ToString().c_str()) << std::endl;
			}
		}
		int skip_day = 0;
		long long compact_total = 0;
		long long compact_count = 0;
		time_t lastti = time(nullptr);
		while(true){
			time_t ti = time(nullptr);
#ifdef __GNUC__
			tm __t;
			tm *t = localtime_r(&ti,&__t);
#else
			tm __t;
			tm* t = &__t;
			localtime_s(t, &ti);
#endif
			int h = t->tm_hour;
			int d = t->tm_mday;




			if(h > 2 && h < 6 && d != skip_day){
				leveldb::Iterator* it = nullptr;
				{
					it = ldb->NewIterator(ropt);
					if(it == nullptr){
						log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
					}else{
						if(cursor.empty()){
							log(lyramilk::log::trace,__FUNCTION__) << D("自动整理开始") << std::endl;
							it->SeekToFirst();
						}else{
							it->Seek(cursor);
						}
						if(it->Valid()){
							int result = 0;
							for(;it->Valid() && result < 300000;it->Next()){
								++result;
								if(lastti != ti && result > 200) break;
							}
							compact_total += result;
							compact_count += result;
							if(it->Valid()){
								lyramilk::data::string end_key = it->key().ToString();
								leveldb::Slice range_start = cursor;
								leveldb::Slice range_end = end_key;
								ldb->CompactRange(&range_start,&range_end);
								cursor = end_key;
								skip_day = 0;

								if(lastti != ti){
									log(lyramilk::log::trace,__FUNCTION__) << D("自动整理中... 整理了%lld条",compact_count) << std::endl;
									lastti = ti;
									compact_count = 0;
								}
							}else{
								if(compact_count != 0){
									log(lyramilk::log::trace,__FUNCTION__) << D("自动整理中... 整理了%lld条",compact_count) << std::endl;
									lastti = ti;
									compact_count = 0;
								}
								leveldb::Slice range_start = cursor;
								ldb->CompactRange(&range_start,nullptr);
								cursor.clear();
								skip_day = d;
								log(lyramilk::log::trace,__FUNCTION__) << D("自动整理完成，共整理%lld条",compact_total) << std::endl;
								ldb->Delete(wopt,lkey);
							}
						}else{
							skip_day = d;
							cursor.clear();
							ldb->Delete(wopt,lkey);
						}

						if (it) delete it;
					}
				}
				usleep(10000);
			}else{
				if(compact_count != 0){
					log(lyramilk::log::trace,__FUNCTION__) << D("自动整理中... 整理了%lld条",compact_count) << std::endl;
					lastti = ti;
					compact_count = 0;
				}

				if (lastcur != cursor){
					leveldb::Status ldbs = ldb->Put(wopt,lkey,cursor);
					if(ldbs.ok()){
						log(lyramilk::log::trace,__FUNCTION__) << D("写入自动整理进度:\"%s\"",encode_for_print(cursor).c_str()) << std::endl;
					}else{
						log(lyramilk::log::error,__FUNCTION__) << D("写入自动整理进度出错:%s",ldbs.ToString().c_str()) << std::endl;
					}
					lastcur = cursor;
				}
				sleep(60 - t->tm_sec);
				//log(lyramilk::log::debug,__FUNCTION__) << D("不满足自动整理条件") << std::endl;
			}


		}

		return nullptr;
	}

	class leveldb_standard_comparator:public leveldb::Comparator
	{
	  public:
		leveldb_standard_comparator()
		{}

		virtual ~leveldb_standard_comparator()
		{}

		virtual int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const
		{
			return cavedb_key::Compare(a,b);
		}

		virtual const char* Name() const
		{
			return "cavedb.KeyComparator";
		}

		virtual void FindShortestSeparator(std::string* start,const leveldb::Slice& limit) const
		{
			cavedb_key::FindShortestSeparator(start,limit);
		}

		virtual void FindShortSuccessor(std::string* key) const
		{
			cavedb_key::FindShortSuccessor(key);
		}

	};

	bool leveldb_store::open_leveldb(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing)
	{
		leveldb::Options opt;

		int block_size = 32;		//KB	16
		int write_buffer_size = 64;	//MB	16
		int max_open_files = cache_size_MB / 1024 * 300;
		if(max_open_files < 500){
			max_open_files = 500;
		}
		if(max_open_files > 4000){
			max_open_files = 4000;
		}

		opt.create_if_missing = create_if_missing;
		opt.max_open_files = max_open_files;
		opt.filter_policy = leveldb::NewBloomFilterPolicy(12);
		opt.block_cache = leveldb::NewLRUCache(cache_size_MB * 1024 * 1024);
		opt.block_size = block_size * 1024;
		opt.write_buffer_size = write_buffer_size * 1024 * 1024;
		opt.max_file_size = 32 * 1024 * 1024;
		opt.compression = leveldb::kSnappyCompression;

		static leveldb_standard_comparator cmr;
		opt.comparator = &cmr;



		leveldb::DB* ldb = nullptr;
		leveldb::Status ldbs = leveldb::DB::Open(opt,leveldbpath.c_str(),&ldb);
		if(ldb == nullptr){
			log(lyramilk::log::error,__FUNCTION__) << D("初始化leveldb失败%s",ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		if(!ldbs.ok()){
			delete ldb;
			ldb = nullptr;
			log(lyramilk::log::error,__FUNCTION__) << D("初始化leveldb失败%s",ldbs.ToString().c_str()) << std::endl;
			return false;
		}

		leveldb::ReadOptions ropt;

		if(ldbs.ok() || ldbs.IsNotFound()){
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.max_open_files=" << opt.max_open_files << std::endl;
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.block_size=" << opt.block_size << std::endl;
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.write_buffer_size=" << opt.write_buffer_size << std::endl;
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.max_file_size=" << opt.max_file_size << std::endl;
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.compression=" << opt.compression << std::endl;
			this->ldb = ldb;

			pthread_t thread;
			if(pthread_create(&thread,NULL,(void* (*)(void*))thread_auto_compact,ldb) == 0){
				pthread_detach(thread);
			}
			return true;
		}
		delete ldb;
		ldb = nullptr;
		log(lyramilk::log::error,__FUNCTION__) << D("初始化leveldb失败%s",ldbs.ToString().c_str()) << std::endl;
		return false;
	}

	bool leveldb_store::save_sync_info(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset) const
	{
		lyramilk::data::string repkey = ".sync.key.";
		repkey += masterid;

		cavedb_key pk;
		pk.type = cavedb_key::s_native;
		pk.key = repkey;
		std::string lkey = cavedb_key::stringify(pk);

		lyramilk::data::string stlsync_info((const char*)&offset,sizeof(lyramilk::data::uint64));
		stlsync_info.append(replid);

		static leveldb::WriteOptions wopt;
		leveldb::Status ldbs = ldb->Put(wopt,lkey,stlsync_info);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("保存同步位置失败 %s",ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}

	bool leveldb_store::get_sync_info(const lyramilk::data::string& masterid,lyramilk::data::string* replid,lyramilk::data::uint64* offset) const
	{
		if(replid == nullptr || offset == nullptr) return false;

		lyramilk::data::string repkey = ".sync.key.";
		repkey += masterid;

		cavedb_key pk;
		pk.type = cavedb_key::s_native;
		pk.key = repkey;
		std::string lkey = cavedb_key::stringify(pk);

		std::string stlsync_info;
		leveldb::Status ldbs = ldb->Get(ropt,lkey,&stlsync_info);

		if(ldbs.IsNotFound()){
			*offset = 0;
			replid->clear();
			return true;
		}

		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("获取同步位置失败 %s",ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		*replid = lyramilk::data::str(stlsync_info.substr(sizeof(lyramilk::data::uint64)));
		stlsync_info.copy((char*)offset,sizeof(lyramilk::data::uint64));
		return true;
	}


	bool leveldb_store::check_command(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen,const command_sepc& cmdspec)
	{
		if(!(cmdspec.flag&command_sepc::skip_monitor)){
			if(cmdspec.flag&command_sepc::readonly){
				++rspeed;
			}else{
				++wspeed;
			}
		}
		if(ldb == nullptr) return false;

		return true;
	}


		/*
	void leveldb_store::after_command(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen,const command_sepc& cmdspec,cmdstatus retcs)
	{
		if(!(cmdspec.flag&command_sepc::readonly)){
			if(blog) blog->append(args);
		}
	}
*/




	cmdstatus leveldb_store::hgetall(const lyramilk::data::string& key,lyramilk::data::stringdict* ret) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::stringdict result;

		lyramilk::data::string result_cursor;
		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			}else{
				lyramilk::data::string keep_prefix = cavedb_key::make_key_prefix(key);
				it->Seek(keep_prefix);
				cavedb_key spack;
				for(;it->Valid();it->Next()){
					if(!it->key().starts_with(keep_prefix)) break;
					cavedb_key spack;
					if(cavedb_key::parse(it->key(),&spack)){
						if(spack.type != cavedb_key::s_hash) continue;
						result[spack.hash.field.ToString()] = lyramilk::data::str(it->value().ToString());
					}
				}

				if (it) delete it;
			}
		}
		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,__FUNCTION__) << D("命令 hgetall %.*s 耗时%.3f",key.size(),key.c_str(),double(nsec) / 1000000) << std::endl;
		}

		*ret = result;
		return cmdstatus::cs_data;
	}

	cmdstatus leveldb_store::hget(const lyramilk::data::string& key,const lyramilk::data::string& field,lyramilk::data::string* ret) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		cavedb_key pk;
		pk.type = cavedb_key::s_hash;

		pk.key = key;
		pk.hash.field.assign(field.data(),field.size());

		std::string lkey = cavedb_key::stringify(pk);

		std::string result;
		leveldb::Status ldbs = ldb->Get(ropt,lkey,&result);

		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,__FUNCTION__) << D("命令 hget %.*s,%.*s 耗时%.3f",key.size(),key.c_str(),field.size(),field.c_str(),double(nsec) / 1000000) << std::endl;
		}

		if(ldbs.ok()){
			*ret = result;
			return cmdstatus::cs_data;
		}
		if(ldbs.IsNotFound()){
			ret->clear();
			return cmdstatus::cs_data_not_found;
		}

		log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
		*ret = ldbs.ToString();
		return cmdstatus::cs_error;
	}

	cmdstatus leveldb_store::hscan(const lyramilk::data::string& key,const lyramilk::data::string& cursor,lyramilk::data::uint64 count,lyramilk::data::string* nextcursor,lyramilk::data::strings* result) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
				return cmdstatus::cs_error;
			}else{
				lyramilk::data::string keep_prefix = cavedb_key::make_key_prefix(key);
				if(cursor == ""){
					it->Seek(keep_prefix);
				}else{
					it->Seek(cursor);
					cavedb_key sb;
					if(cavedb_key::parse(cursor,&sb) && sb.type == cavedb_key::s_hash){
						while(it->Valid()){
							cavedb_key sa;
							if(cavedb_key::parse(it->key(),&sa) && sa.type == cavedb_key::s_hash){
								if(sa.hash.field == sb.hash.field){
									it->Next();
									continue;
								}
							}
							break;
						}
					}else{
						//log(lyramilk::log::warning,__FUNCTION__) << D("光标指向的不是%s类型","set") << std::endl;
						return cmdstatus::cs_error;
					}
				}
				if(it->Valid()){
					cavedb_key spack;
					for(lyramilk::data::uint64 i=0;it->Valid() && i<count;it->Next()){
						if(!it->key().starts_with(keep_prefix)) break;

						if(cavedb_key::parse(it->key(),&spack) && spack.type == cavedb_key::s_hash){
							result->push_back(spack.hash.field.ToString());
							++i;

							if(i == count){
								if(nextcursor) *nextcursor = it->key().ToString();
								break;
							}
						}else{
							break;
						}
					}
				}

				if (it) delete it;
			}
		}
		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,"hscan") << D("命令 hscan %.*s %.*s 耗时%.3f",key.size(),key.c_str(),cursor.size(),cursor.c_str(),double(nsec) / 1000000) << std::endl;
		}
		return cmdstatus::cs_data;
	}

	cmdstatus leveldb_store::smembers(const lyramilk::data::string& key,lyramilk::data::strings* ret) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::strings result;

		lyramilk::data::string result_cursor;
		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
				return cmdstatus::cs_error;
			}else{
				lyramilk::data::string keep_prefix = cavedb_key::make_key_prefix(key);
				it->Seek(keep_prefix);
				cavedb_key spack;
				for(;it->Valid();it->Next()){
					if(!it->key().starts_with(keep_prefix)) break;
					cavedb_key spack;
					if(cavedb_key::parse(it->key(),&spack)){
						if(spack.type != cavedb_key::s_set) continue;
						result.push_back(spack.set.member.ToString());
					}
				}

				if (it) delete it;
			}
		}
		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,__FUNCTION__) << D("命令 smembers %.*s 耗时%.3f",key.size(),key.c_str(),double(nsec) / 1000000) << std::endl;
		}

		*ret = result;
		return cmdstatus::cs_data;
	}
	cmdstatus leveldb_store::sscan(const lyramilk::data::string& key,const lyramilk::data::string& cursor,lyramilk::data::uint64 count,lyramilk::data::string* nextcursor,lyramilk::data::strings* result) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
				return cmdstatus::cs_error;
			}else{
				lyramilk::data::string keep_prefix = cavedb_key::make_key_prefix(key);
				if(cursor == ""){
					it->Seek(keep_prefix);
				}else{
					it->Seek(cursor);
					cavedb_key sb;
					if(cavedb_key::parse(cursor,&sb) && sb.type == cavedb_key::s_set){
						while(it->Valid()){
							cavedb_key sa;
							if(cavedb_key::parse(it->key(),&sa) && sa.type == cavedb_key::s_set){
								if(sa.set.member == sb.set.member){
									it->Next();
									continue;
								}
							}
							break;
						}
					}else{
						//log(lyramilk::log::warning,__FUNCTION__) << D("光标指向的不是%s类型","set") << std::endl;
						return cmdstatus::cs_error;
					}
				}
				if(it->Valid()){
					cavedb_key spack;
					for(lyramilk::data::uint64 i=0;it->Valid() && i<count;it->Next()){
						if(!it->key().starts_with(keep_prefix)) break;

						if(cavedb_key::parse(it->key(),&spack) && spack.type == cavedb_key::s_set){
							result->push_back(spack.set.member.ToString());
							++i;

							if(i == count){
								if(nextcursor) *nextcursor = it->key().ToString();
								break;
							}
						}else{
							break;
						}
					}
				}

				if (it) delete it;
			}
		}
		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,"sscan") << D("命令 sscan %.*s %.*s 耗时%.3f",key.size(),key.c_str(),cursor.size(),cursor.c_str(),double(nsec) / 1000000) << std::endl;
		}
		return cmdstatus::cs_data;
	}

	cmdstatus leveldb_store::scan(const lyramilk::data::string& cursor,lyramilk::data::uint64 count,lyramilk::data::string* nextcursor,lyramilk::data::strings* result) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
				return cmdstatus::cs_error;
			}else{
				if(cursor == ""){
					it->SeekToFirst();
				}else{
					it->Seek(cursor);
					while(it->Valid()){
						cavedb_key sa;
						cavedb_key sb;
						if(cavedb_key::parse(it->key(),&sa) && cavedb_key::parse(cursor,&sb)){
							if(sa.key == sb.key){
								it->Next();
								continue;
							}
						}
						break;
					}
				}
				if(it->Valid()){
					lyramilk::data::string skip_prefix;
					skip_prefix.push_back(cavedb_key::magiceof);

					cavedb_key spack;
					for(lyramilk::data::uint64 i=0;it->Valid() && i<count;){
						if(it->key().starts_with(skip_prefix)) continue;
						if(cavedb_key::parse(it->key(),&spack) && spack.type != cavedb_key::s_native){
							skip_prefix = cavedb_key::make_key_prefix(spack.key);

							result->push_back(spack.key.ToString());
							++i;

							if(i == count){
								if(nextcursor) *nextcursor = it->key().ToString();

								if(spack.type == cavedb_key::s_string){
									it->Next();
								}else{
									it->Seek(cavedb_key::make_key_eof(spack.key));
								}
								break;
							}

							if(spack.type == cavedb_key::s_string){
								it->Next();
							}else{
								it->Seek(cavedb_key::make_key_eof(spack.key));
							}
						}else{
							it->Next();
						}
					}
				}

				if (it) delete it;
			}
		}
		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,"scan") << D("命令 scan %.*s 耗时%.3f",cursor.size(),cursor.c_str(),double(nsec) / 1000000) << std::endl;
		}
		return cmdstatus::cs_data;
	}
	
	cmdstatus leveldb_store::type(const lyramilk::data::string& key,cavedb_key_type* result) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		
		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
				return cmdstatus::cs_error;
			}else{
				*result = cavedb_key::s_none;
				lyramilk::data::string prefix = cavedb_key::make_key_prefix(key);
				it->Seek(prefix);
				if(it->Valid()){
					if(it->key().starts_with(prefix)){
						cavedb_key spack;
						if(cavedb_key::parse(it->key(),&spack)){
							*result = spack.type;
						}
					}
				}

				if (it) delete it;
			}
		}
		
		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,"type") << D("命令 type %.*s 耗时%.3f",key.size(),key.c_str(),double(nsec) / 1000000) << std::endl;
		}
		
		return cmdstatus::cs_data;
	}

}}
