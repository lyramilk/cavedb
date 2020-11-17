#include "leveldb_standard.h"
#include <libmilk/log.h>
#include <libmilk/dict.h>
#include <libmilk/testing.h>
#include <libmilk/codes.h>
#include <libmilk/hash.h>

#include <leveldb/db.h>
#include <leveldb/filter_policy.h>
#include <leveldb/cache.h>
#include <leveldb/write_batch.h>
#include <leveldb/comparator.h>
#include <leveldb/slice.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

namespace lyramilk{ namespace cave
{
	lyramilk::log::logss static log(lyramilk::klog,"lyramilk.cave.store.leveldb_standard");
	const std::string leveldb_standard::cfver = "2_leveldb_standard";
	extern leveldb::ReadOptions ropt;
	extern leveldb::WriteOptions wopt;

	class leveldb_standard_comparator:public leveldb::Comparator
	{
	  public:
		leveldb_standard_comparator()
		{}

		virtual ~leveldb_standard_comparator()
		{}

		virtual int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const
		{
			return redis_pack::Compare(a,b);
		}

		virtual const char* Name() const
		{
			return "cavedb.KeyComparator";
		}

		virtual void FindShortestSeparator(std::string* start,const leveldb::Slice& limit) const
		{
			redis_pack::FindShortestSeparator(start,limit);
		}

		virtual void FindShortSuccessor(std::string* key) const
		{
			redis_pack::FindShortSuccessor(key);
		}

	};

	void inline save_process(leveldb::WriteBatch& batch,const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
	{
		if(offset == 0 && replid.empty()) return;
		redis_pack pk;
		pk.type = redis_pack::s_native;

		lyramilk::data::string repkey;
		if(masterid.empty()){
			repkey = ".sync.key";
		}else{
			repkey = ".sync.key.";
			repkey += masterid;
		}

		pk.key = repkey;
		std::string lkey = redis_pack::pack(&pk);

		lyramilk::data::string str((const char*)&offset,sizeof(lyramilk::data::uint64));
		str.append(replid);
		batch.Put(lkey,leveldb::Slice(str.c_str(),str.size()));
	}

	bool leveldb_standard::notify_idle(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata)
	{
		/*
		leveldb::WriteBatch batch;
		save_process(batch,replid,offset);
		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}*/
		return true;
	}

	bool leveldb_standard::notify_psync(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata)
	{
		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);
		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}

	bool leveldb_standard::notify_flushdb(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		leveldb::Iterator* it = ldb->NewIterator(ropt);
		if(it == nullptr){
			log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			return false;
		}

		for(it->SeekToFirst();it->Valid();it->Next()){
			ldb->Delete(wopt,it->key());
		}
		if (it) delete it;

		ldb->CompactRange(nullptr,nullptr);
		return true;
	}

	bool leveldb_standard::notify_flushall(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		leveldb::Iterator* it = ldb->NewIterator(ropt);
		if(it == nullptr){
			log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			return false;
		}

		for(it->SeekToFirst();it->Valid();it->Next()){
			ldb->Delete(wopt,it->key());
		}
		if (it) delete it;

		ldb->CompactRange(nullptr,nullptr);
		return true;
	}

	bool leveldb_standard::notify_del(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		//log(lyramilk::log::error,__FUNCTION__) << D("未实现%s函数，这在ssdb中不应该出现","del") << std::endl;
		//return false;
		lyramilk::data::string key = args[1].str();

		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);


		lyramilk::data::string result_cursor;
		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			}else{
				lyramilk::data::string keep_prefix = redis_pack::make_key_prefix(key);
				it->Seek(keep_prefix);
				if(it->Valid()){
					for(;it->Valid();it->Next()){
						if(!it->key().starts_with(keep_prefix)) break;
						batch.Delete(it->key());
					}
				}

				if (it) delete it;
			}
		}


		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}

	bool leveldb_standard::notify_move(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		log(lyramilk::log::error,__FUNCTION__) << D("未实现%s函数，这在ssdb中不应该出现","move") << std::endl;
		return false;
	}

	bool leveldb_standard::notify_pexpireat(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return false;
	}

	bool leveldb_standard::notify_persist(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return false;
	}

	bool leveldb_standard::notify_rename(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		log(lyramilk::log::error,__FUNCTION__) << D("未实现%s函数，这在ssdb中不应该出现","rename") << std::endl;
		return false;
	}

	bool leveldb_standard::notify_hset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		redis_pack pk;
		pk.type = redis_pack::s_hash;

		lyramilk::data::string key = args[1].str();
		lyramilk::data::string field = args[2].str();
		pk.key = key;
		pk.hash.field.assign(field.data(),field.size());

		std::string lkey = redis_pack::pack(&pk);

		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);
		batch.Put(lkey,lyramilk::data::str(args[3].str()));
		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}

	bool leveldb_standard::notify_hmset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);


		redis_pack pk;
		pk.type = redis_pack::s_hash;
		lyramilk::data::string key = args[1].str();

		lyramilk::data::uint64 c = args.size();
		if(args.size() % 2 != 0){
			--c;
		}

		for(lyramilk::data::uint64 i=2;i<c;i+=2){
			lyramilk::data::string field = args[i].str();
			pk.key = key;
			pk.hash.field.assign(field.data(),field.size());

			std::string lkey = redis_pack::pack(&pk);
			batch.Put(lkey,lyramilk::data::str(args[i + 1].str()));
		}

		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}

	bool leveldb_standard::notify_hdel(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);

		redis_pack pk;
		pk.type = redis_pack::s_hash;
		lyramilk::data::string key = args[1].str();
		pk.key = key;

		for(std::size_t idx = 2;idx < args.size();++idx){
			lyramilk::data::string field = args[idx].str();
			pk.hash.field.assign(field.data(),field.size());
			std::string lkey = redis_pack::pack(&pk);
			batch.Delete(lkey);
		}

		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}


	bool leveldb_standard::notify_set(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		redis_pack pk;
		pk.type = redis_pack::s_string;

		lyramilk::data::string key = args[1].str();
		pk.key = key;
		std::string lkey = redis_pack::pack(&pk);

		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);
		batch.Put(lkey,lyramilk::data::str(args[2].str()));
		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}

	bool leveldb_standard::notify_ssdb_del(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		redis_pack pk;
		pk.type = redis_pack::s_string;

		lyramilk::data::string key = args[1].str();
		pk.key = key;
		std::string lkey = redis_pack::pack(&pk);

		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);
		batch.Delete(lkey);
		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}


	const static unsigned long long SEQ_0 = 0x3fffffffffffffffull;


	bool leveldb_standard::notify_ssdb_qset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		redis_pack pk;
		pk.type = redis_pack::s_list;

		lyramilk::data::string key = args[1].str();
		pk.key = key;
		pk.list.seq = args[2].conv(0l);

		std::string lkey = redis_pack::pack(&pk);

		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);
		batch.Delete(lkey);
		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}

	bool leveldb_standard::notify_lpop(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		log(lyramilk::log::error,__FUNCTION__) << D("未实现%s函数","lpop") << std::endl;
		return false;
	}

	bool leveldb_standard::notify_rpop(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		log(lyramilk::log::error,__FUNCTION__) << D("未实现%s函数","rpop") << std::endl;
		return false;
	}

	bool leveldb_standard::notify_zadd(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		redis_pack pk;
		pk.type = redis_pack::s_zset_m2s;

		lyramilk::data::string key = args[1].str();
		double score = args[2];
		lyramilk::data::string value = args[3].str();
		pk.key = key;
		pk.zset.score = score;
		pk.zset.member.assign(value.data(),value.size());


		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);

		{
			//写入 s_zset_m2s
			std::string lkey = redis_pack::pack(&pk);
			std::string sscore((const char*)&score,sizeof(double));
			batch.Put(lkey,sscore);
		}
		{
			//写入 s_zset
			pk.type = redis_pack::s_zset;
			std::string lkey = redis_pack::pack(&pk);
			batch.Put(lkey,"1");
		}

		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}

	bool leveldb_standard::notify_zrem(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		{
		redis_pack pk;
		pk.type = redis_pack::s_zset_m2s;

		lyramilk::data::string key = args[1].str();
		lyramilk::data::string value = args[2].str();
		pk.key = key;
		pk.zset.member.assign(value.data(),value.size());

		std::string lkey = redis_pack::pack(&pk);
		std::string data;
		leveldb::Status ldbs = ldb->Get(ropt,lkey,&data);


		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);

		if(ldbs.ok()){
			double score = 0;
			data.copy((char*)&score,sizeof(double));
			pk.zset.score = score;
		}

		{
			//删除 s_zset_m2s
			std::string lkey = redis_pack::pack(&pk);
			batch.Delete(lkey);
		}
		{
			//删除 s_zset
			pk.type = redis_pack::s_zset;
			std::string lkey = redis_pack::pack(&pk);
			batch.Delete(lkey);
		}

		ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		}
		return true;
	}

	leveldb_standard::leveldb_standard()
	{
		ldb = nullptr;
	}

	leveldb_standard::~leveldb_standard()
	{
		if(ldb)delete ldb;
	}

	bool leveldb_standard::get_sync_info(const lyramilk::data::string& masterid,lyramilk::data::string* replid,lyramilk::data::uint64* offset) const
	{
		if(replid == nullptr || offset == nullptr) return false;


		lyramilk::data::string repkey;
		if(masterid.empty()){
			repkey = ".sync.key";
		}else{
			repkey = ".sync.key.";
			repkey += masterid;
		}

		redis_pack pk;
		pk.type = redis_pack::s_native;
		pk.key = repkey;
		std::string lkey = redis_pack::pack(&pk);


		std::string stlsync_info;
		leveldb::Status ldbs = ldb->Get(ropt,lkey,&stlsync_info);

		if(ldbs.IsNotFound()){
			*offset = 0;
			replid->clear();
			return true;
		}

		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("获取同步位置失败2 %s",ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		*replid = lyramilk::data::str(stlsync_info.substr(sizeof(lyramilk::data::uint64)));
		stlsync_info.copy((char*)offset,sizeof(lyramilk::data::uint64));
		return true;
	}

	bool leveldb_standard::compact()
	{
		ldb->CompactRange(nullptr,nullptr);
		return true;
	}


	bool leveldb_standard::zrange(const lyramilk::data::string& key,lyramilk::data::int64 start,lyramilk::data::int64 stop,bool withscore,lyramilk::data::strings* result) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		rspeed_on_read();

		lyramilk::data::int64 total = 0;//zcard(key);
		if(start < 0 || stop < 0){
			total = (lyramilk::data::int64)zcard(key);
		}

		lyramilk::data::uint64 rstart = 0;
		if(start >= 0){
			rstart = start;
		}else if(total - start > 0){
			rstart = total + start;
		}else{
			rstart = 0;
		}
		lyramilk::data::uint64 rstop = 0;
		if(stop >= 0){
			rstop = stop;
		}else if(total - stop > 0){
			rstop = total + stop;
		}else{
			rstop = total - 1;
		}

		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			}else{
				lyramilk::data::string keep_prefix = redis_pack::make_key_prefix(key);
				it->Seek(keep_prefix);

				if(it->Valid()){
					redis_pack spack;
					for(lyramilk::data::uint64 i=0;it->Valid();it->Next()){
						if(!it->key().starts_with(keep_prefix)) break;
						if(redis_pack::unpack(&spack,it->key()) && spack.type == redis_pack::s_zset){
							if(i < rstart){
								++i;
								continue;
							}
							++i;
							result->push_back(spack.zset.member.ToString());
							if(withscore){
								result->push_back(lyramilk::data::str(spack.zset.score));
							}
							if(i > rstop) break;
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
			log(lyramilk::log::warning,"zrange") << D("命令 zrange %.*s %lld %lld 耗时%.3f",key.size(),key.c_str(),start,stop,double(nsec) / 1000000) << std::endl;
		}
		return true;
	}

	lyramilk::data::string leveldb_standard::zscan(const lyramilk::data::string& key,const lyramilk::data::string& current,lyramilk::data::uint64 count,lyramilk::data::strings* result) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		rspeed_on_read();

		lyramilk::data::string result_cursor;
		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			}else{
				lyramilk::data::string keep_prefix = redis_pack::make_key_prefix(key);
				if(current == ""){
					it->Seek(keep_prefix);
				}else{
					it->Seek(current);
					while(it->Valid()){
						redis_pack sa;
						redis_pack sb;
						if(redis_pack::unpack(&sa,it->key()) && redis_pack::unpack(&sb,current)){
							if(sa.zset.member == sb.zset.member){
								it->Next();
								continue;
							}
						}
						break;
					}
				}
				if(it->Valid()){
					redis_pack spack;
					for(lyramilk::data::uint64 i=0;it->Valid() && i<count;it->Next()){
						if(!it->key().starts_with(keep_prefix)) break;

						if(redis_pack::unpack(&spack,it->key()) && spack.type == redis_pack::s_zset){
							result->push_back(spack.zset.member.ToString());
							result->push_back(lyramilk::data::str(spack.zset.score));
							++i;

							if(i == count){
								result_cursor = it->key().ToString();
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
			log(lyramilk::log::warning,"zscan") << D("命令 zscan %.*s %.*s 耗时%.3f",key.size(),key.c_str(),current.size(),current.c_str(),double(nsec) / 1000000) << std::endl;
		}
		return result_cursor;
	}


	lyramilk::data::uint64 leveldb_standard::zcard(const lyramilk::data::string& key) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		rspeed_on_read();

		lyramilk::data::uint64 result = 0;
		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			}else{
				lyramilk::data::string keep_prefix = redis_pack::make_key_prefix(key);
				it->Seek(keep_prefix);

				if(it->Valid()){
					for(;it->Valid();it->Next()){
						if(!it->key().starts_with(keep_prefix)) break;
						redis_pack spack;
						if(redis_pack::unpack(&spack,it->key())){
							if(spack.type != redis_pack::s_zset) break;;
							++result;
						}
					}
				}

				if (it) delete it;
			}
		}
		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,"zcard") << D("命令 zcard %.*s 耗时%.3f",key.size(),key.c_str(),double(nsec) / 1000000) << std::endl;
		}
		return result;
	}

	bool leveldb_standard::get(const lyramilk::data::string& key,lyramilk::data::string* value) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		rspeed_on_read();

		redis_pack pk;
		pk.type = redis_pack::s_string;

		pk.key = key;
		std::string lkey = redis_pack::pack(&pk);

		std::string result;
		leveldb::Status ldbs = ldb->Get(ropt,lkey,&result);

		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,"get") << D("命令 get %.*s 耗时%.3f",key.size(),key.c_str(),double(nsec) / 1000000) << std::endl;
		}

		if(!ldbs.ok()){
			return false;
		}
		if(value) *value = lyramilk::data::str(result);
		return true;
	}

	bool leveldb_standard::hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		rspeed_on_read();

		redis_pack pk;
		pk.type = redis_pack::s_hash;

		pk.key = key;
		pk.hash.field.assign(field.data(),field.size());

		std::string lkey = redis_pack::pack(&pk);

		std::string result;
		leveldb::Status ldbs = ldb->Get(ropt,lkey,&result);

		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,"hexist") << D("命令 hexist %.*s,%.*s 耗时%.3f",key.size(),key.c_str(),field.size(),field.c_str(),double(nsec) / 1000000) << std::endl;
		}
		return ldbs.ok();
	}

	lyramilk::data::string leveldb_standard::hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		rspeed_on_read();

		redis_pack pk;
		pk.type = redis_pack::s_hash;

		pk.key = key;
		pk.hash.field.assign(field.data(),field.size());

		std::string lkey = redis_pack::pack(&pk);

		std::string result;
		leveldb::Status ldbs = ldb->Get(ropt,lkey,&result);
		if(!ldbs.ok()){
			result.clear();
		}

		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,"hget") << D("命令 hget %.*s,%.*s 耗时%.3f",key.size(),key.c_str(),field.size(),field.c_str(),double(nsec) / 1000000) << std::endl;
		}
		return result;
	}

	bool leveldb_standard::hget(const lyramilk::data::string& key,const lyramilk::data::string& field,lyramilk::data::string* value) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		rspeed_on_read();

		redis_pack pk;
		pk.type = redis_pack::s_hash;

		pk.key = key;
		pk.hash.field.assign(field.data(),field.size());

		std::string lkey = redis_pack::pack(&pk);

		std::string result;
		leveldb::Status ldbs = ldb->Get(ropt,lkey,&result);
		if(value)*value = result;

		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,"hget") << D("命令 hget %.*s,%.*s 耗时%.3f",key.size(),key.c_str(),field.size(),field.c_str(),double(nsec) / 1000000) << std::endl;
		}
		return ldbs.ok();
	}

	lyramilk::data::stringdict leveldb_standard::hgetall(const lyramilk::data::string& key) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		rspeed_on_read();

		lyramilk::data::stringdict result;

		lyramilk::data::string result_cursor;
		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			}else{
				lyramilk::data::string keep_prefix = redis_pack::make_key_prefix(key);
				it->Seek(keep_prefix);
				redis_pack spack;
				for(;it->Valid();it->Next()){
					if(!it->key().starts_with(keep_prefix)) break;
					redis_pack spack;
					if(redis_pack::unpack(&spack,it->key())){
						if(spack.type != redis_pack::s_hash) continue;
						result[spack.hash.field.ToString()] = lyramilk::data::str(it->value().ToString());
					}
				}

				if (it) delete it;
			}
		}
		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,"hgetall") << D("命令 hgetall %.*s 耗时%.3f",key.size(),key.c_str(),double(nsec) / 1000000) << std::endl;
		}
		return result;
	}


	lyramilk::data::string leveldb_standard::scan(const lyramilk::data::string& current,lyramilk::data::uint64 count,lyramilk::data::strings* result) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		rspeed_on_read();

		lyramilk::data::string result_cursor;
		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			}else{
				if(current == ""){
					it->SeekToFirst();
				}else{
					it->Seek(current);
					while(it->Valid()){
						redis_pack sa;
						redis_pack sb;
						if(redis_pack::unpack(&sa,it->key()) && redis_pack::unpack(&sb,current)){
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
					skip_prefix.push_back(redis_pack::magiceof);

					redis_pack spack;
					for(lyramilk::data::uint64 i=0;it->Valid() && i<count;it->Next()){
						if(it->key().starts_with(skip_prefix)) continue;
						if(redis_pack::unpack(&spack,it->key()) && spack.type != redis_pack::s_native){
							skip_prefix = redis_pack::make_key_prefix(spack.key);
							result->push_back(spack.key.ToString());
							++i;

							if(i == count){
								result_cursor = it->key().ToString();
								break;
							}
						}
					}
				}

				if (it) delete it;
			}
		}
		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,"scan") << D("命令 scan %.*s 耗时%.3f",current.size(),current.c_str(),double(nsec) / 1000000) << std::endl;
		}
		return result_cursor;
	}

	lyramilk::data::string leveldb_standard::hscan(const lyramilk::data::string& key,const lyramilk::data::string& current,lyramilk::data::uint64 count,lyramilk::data::strings* result) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		rspeed_on_read();

		lyramilk::data::string result_cursor;
		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			}else{
				lyramilk::data::string keep_prefix = redis_pack::make_key_prefix(key);
				if(current == ""){
					it->Seek(keep_prefix);
				}else{
					it->Seek(current);
					while(it->Valid()){
						redis_pack sa;
						redis_pack sb;
						if(redis_pack::unpack(&sa,it->key()) && redis_pack::unpack(&sb,current)){
							if(sa.hash.field == sb.hash.field){
								it->Next();
								continue;
							}
						}
						break;
					}
				}
				if(it->Valid()){
					lyramilk::data::string lastfield;
					redis_pack spack;
					for(lyramilk::data::uint64 i=0;it->Valid() && i<count;it->Next()){
						if(!it->key().starts_with(keep_prefix)) break;

						if(redis_pack::unpack(&spack,it->key()) && spack.type == redis_pack::s_hash){
							if(spack.hash.field == lastfield){
								continue;
							}

							result->push_back(spack.hash.field.ToString());
							result->push_back(it->value().ToString());
							++i;

							if(i == count){
								result_cursor = it->key().ToString();
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
			log(lyramilk::log::warning,"hscan") << D("命令 hscan %.*s %.*s 耗时%.3f",key.size(),key.c_str(),current.size(),current.c_str(),double(nsec) / 1000000) << std::endl;
		}
		return result_cursor;
	}

	lyramilk::data::uint64 leveldb_standard::hlen(const lyramilk::data::string& key) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		rspeed_on_read();

		lyramilk::data::uint64 result = 0;

		lyramilk::data::string result_cursor;
		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			}else{
				lyramilk::data::string keep_prefix = redis_pack::make_key_prefix(key);
				it->Seek(keep_prefix);
				if(it->Valid()){
					for(;it->Valid();it->Next()){
						if(!it->key().starts_with(keep_prefix)) break;
						++result;
					}
				}

				if (it) delete it;
			}
		}
		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,"hlen") << D("命令 hlen %.*s 耗时%.3f",key.size(),key.c_str(),double(nsec) / 1000000) << std::endl;
		}
		return result;
	}

	lyramilk::data::string leveldb_standard::type(const lyramilk::data::string& key) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		rspeed_on_read();

		lyramilk::data::string key_type = "none";
		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			}else{
				lyramilk::data::string prefix = redis_pack::make_key_prefix(key);
				it->Seek(prefix);
				if(it->Valid()){
					if(it->key().starts_with(prefix)){
						redis_pack spack;
						if(redis_pack::unpack(&spack,it->key())){
							if(spack.type == redis_pack::s_string){
								key_type = "string";
							}else if(spack.type == redis_pack::s_hash){
								key_type = "hash";
							}else if(spack.type == redis_pack::s_list){
								key_type = "list";
							}else if(spack.type == redis_pack::s_set){
								key_type = "set";
							}else if(spack.type == redis_pack::s_zset){
								key_type = "zset";
							}
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

		return key_type;
	}

	bool leveldb_standard::is_on_full_sync()
	{
		return on_full_sync();
	}

	lyramilk::data::string leveldb_standard::get_property(const lyramilk::data::string& property)
	{
		std::string result;
		if(ldb->GetProperty(property,&result)){
			return lyramilk::data::str(result);
		}
		return "";
	}

	void* leveldb_standard::thread_auto_compact(leveldb::DB* ldb)
	{
		lyramilk::data::string cursor;
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
							log(lyramilk::log::debug,__FUNCTION__) << D("自动整理开始") << std::endl;
							it->SeekToFirst();
						}else{
							it->Seek(cursor);
						}
						if(it->Valid()){
							int result = 0;
							for(;it->Valid() && result < 300000;it->Next()){
								++result;
								if(lastti != ti) break;
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
									log(lyramilk::log::debug,__FUNCTION__) << D("自动整理中... 整理了%lld条",compact_count) << std::endl;
									lastti = ti;
									compact_count = 0;
								}
							}else{
								leveldb::Slice range_start = cursor;
								ldb->CompactRange(&range_start,nullptr);
								cursor.clear();
								skip_day = d;
								log(lyramilk::log::debug,__FUNCTION__) << D("自动整理完成，共整理%lld条",compact_total) << std::endl;
							}
						}else{
							skip_day = d;
							cursor.clear();
						}

						if (it) delete it;
					}
				}
				usleep(10000);
			}else{
				sleep(60 - t->tm_sec);
				//log(lyramilk::log::debug,__FUNCTION__) << D("不满足自动整理条件") << std::endl;
			}
		}

		return nullptr;
	}

	bool leveldb_standard::open_leveldb(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing)
	{
		lyramilk::data::string flag;
		{
			lyramilk::data::string fflag = leveldbpath + "/cavedb.flag";
			struct stat st = {0};
			if(0 !=::stat(fflag.c_str(),&st)){
				if(errno == ENOENT){
					if(create_if_missing){
						system(("mkdir -p " + leveldbpath).c_str());
						int fd_flag = ::open(fflag.c_str(),O_WRONLY | O_CREAT | O_APPEND,0444);
						if(fd_flag == -1)  return false;
						int w = write(fd_flag,cfver.c_str(),cfver.size());
						::close(fd_flag);
						if(w == (int)cfver.size()){
							flag = cfver;
						}
					}else{
						return false;
					}
				}else{
					return false;
				}
			}else{
				int fd_flag = ::open(fflag.c_str(),O_RDONLY,0444);
				if(fd_flag == -1)  return false;
				char buff[1024];
				int r = read(fd_flag,buff,sizeof(buff));
				::close(fd_flag);
				if(r > 0){
					flag.assign(buff,r);
				}
			}
		}

		if(flag != cfver) return false;

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
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.cfver=" << flag << std::endl;
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.max_open_files=" << opt.max_open_files << std::endl;
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.block_size=" << opt.block_size << std::endl;
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.write_buffer_size=" << opt.write_buffer_size << std::endl;
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.max_file_size=" << opt.max_file_size << std::endl;
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.compression=" << opt.compression << std::endl;
			log(lyramilk::log::debug,__FUNCTION__) << D("cfver=%.*s",cfver.size(),cfver.c_str()) << std::endl;
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

}}
