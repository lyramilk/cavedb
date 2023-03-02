#include "leveldb_store.h"
#include "redis_pack.h"

#include <libmilk/log.h>
#include <libmilk/testing.h>
#include <libmilk/dict.h>

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
	leveldb::WriteOptions wopt;
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





	//leveldb::DB* ldb;

	cmdstatus leveldb_store::on_hgetall(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		/*
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::string key = args[1].str();

		lyramilk::data::map result;

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
			log(lyramilk::log::warning,__FUNCTION__) << D("命令 hgetall %.*s 耗时%.3f",key.size(),key.c_str(),double(nsec) / 1000000) << std::endl;
		}

		*ret = result;
		return cmdstatus::cs_data;
		*/

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
		/*
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::string key = args[1].str();
		lyramilk::data::string field = args[2].str();


		redis_pack pk;
		pk.type = redis_pack::s_hash;

		pk.key = key;
		pk.hash.field.assign(field.data(),field.size());

		std::string lkey = redis_pack::pack(&pk);

		std::string result;
		leveldb::Status ldbs = ldb->Get(ropt,lkey,&result);

		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,__FUNCTION__) << D("命令 hget %.*s,%.*s 耗时%.3f",key.size(),key.c_str(),field.size(),field.c_str(),double(nsec) / 1000000) << std::endl;
		}

		if(ldbs.ok()){
			*ret = result;
			return cmdstatus::cs_ok;
		}
		if(ldbs.IsNotFound()){
			ret->clear();
			return cmdstatus::cs_data;
		}

		log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
		*ret = ldbs.ToString();
		return cmdstatus::cs_error;
		*/

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
		/*
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::string key = args[1].str();

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
			log(lyramilk::log::warning,__FUNCTION__) << D("命令 hgetall %.*s 耗时%.3f",key.size(),key.c_str(),double(nsec) / 1000000) << std::endl;
		}

		ret->type(lyramilk::data::var::t_array);

		lyramilk::data::array& ar = *ret;

		for(lyramilk::data::array::const_iterator it = args.begin() + 2;it!=args.end();++it){
			lyramilk::data::stringdict::iterator cit = result.find(it->str());
			if(cit == result.end()){
				ar.push_back(lyramilk::data::var::nil);
			}else{
				ar.push_back(cit->first);
			}
		}
		return cmdstatus::cs_data;
		*/


		ret->type(lyramilk::data::var::t_array);
		lyramilk::data::array& ar = *ret;

		lyramilk::data::string key = args[1].str();

		for(lyramilk::data::array::const_iterator it = args.begin() + 2;it!=args.end();++it){
			lyramilk::data::string val;
			cmdstatus r = hget(key,it->str(),&val);
			if(r == cmdstatus::cs_data){
				ar.push_back(r);
			}
			/*
			lyramilk::data::stringdict::iterator cit = result.find(it->str());
			if(cit == result.end()){
				ar.push_back(lyramilk::data::var::nil);
			}else{
				ar.push_back(cit->first);
			}*/
		}
		return cmdstatus::cs_data;



	}

	cmdstatus leveldb_store::on_hexist(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		lyramilk::data::string key = args[1].str();
		lyramilk::data::string field = args[2].str();

		redis_pack pk;
		pk.type = redis_pack::s_hash;

		pk.key = key;
		pk.hash.field.assign(field.data(),field.size());

		std::string lkey = redis_pack::pack(&pk);

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

		leveldb::WriteBatch batch;

		redis_pack pk;
		pk.type = redis_pack::s_hash;
		lyramilk::data::string key = args[1].str();

		lyramilk::data::uint64 c = args.size();
		lyramilk::data::uint64 r = 0;
		if(args.size() % 2 != 0){
			--c;
		}

		for(lyramilk::data::uint64 i=2;i<c;i+=2){
			++r;
			lyramilk::data::string field = args[i].str();
			pk.key = key;
			pk.hash.field.assign(field.data(),field.size());

			std::string lkey = redis_pack::pack(&pk);
			batch.Put(lkey,lyramilk::data::str(args[i + 1].str()));
		}

		leveldb::Status ldbs = ldb->Write(wopt,&batch);

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

		leveldb::WriteBatch batch;

		redis_pack pk;
		pk.type = redis_pack::s_hash;
		lyramilk::data::string key = args[1].str();

		lyramilk::data::uint64 c = args.size();
		lyramilk::data::uint64 r = 0;

		for(lyramilk::data::uint64 i=2;i<c;++i){
			++r;
			lyramilk::data::string field = args[i].str();
			pk.key = key;
			pk.hash.field.assign(field.data(),field.size());

			std::string lkey = redis_pack::pack(&pk);
			batch.Delete(lkey);
		}

		leveldb::Status ldbs = ldb->Write(wopt,&batch);

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


	cmdstatus leveldb_store::on_del(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();




		lyramilk::data::string key = args[1].str();



		long ditems = 0;
		leveldb::WriteBatch batch;

		std::string start;
		std::string end;

		lyramilk::data::string result_cursor;
		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			}else{

				lyramilk::data::string keep_prefix = redis_pack::make_key_prefix(key);
				start = keep_prefix;

				it->Seek(keep_prefix);
				if(it->Valid()){
					for(;it->Valid();it->Next()){
						if(!it->key().starts_with(keep_prefix)){
							end = it->key().ToString();
							break;
						}
						batch.Delete(it->key());
						++ditems;
					}
				}

				if (it) delete it;
			}
		}


		leveldb::Status ldbs = ldb->Write(wopt,&batch);
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
				nextseq = blog->find_max();
			}else{
				nextseq = seq;
			}

			//如果key不为空，或key为空且seq为0，且扫描本地数据。
			leveldb::Iterator* it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
				*ret = "create iterator fail";
				return cmdstatus::cs_error;
			}else{
				it->Seek(key);
				for(lyramilk::data::int64 i =0;it->Valid() && i < count;it->Next(),++i){

					redis_pack spack;
					if(redis_pack::unpack(&spack,it->key())){
						if(spack.type == redis_pack::s_hash){
							lyramilk::data::var v;
							ardata.push_back(v);
							ardata.back().type(lyramilk::data::var::t_array);

							lyramilk::data::array& ar = ardata.back();
							ar.push_back("hset");
							ar.push_back(lyramilk::data::str(spack.key.ToString()));
							ar.push_back(lyramilk::data::str(spack.hash.field.ToString()));
							ar.push_back(lyramilk::data::str(it->value().ToString()));
						}
					}
				}

				if(it->Valid()){
					nextkey = lyramilk::data::str(it->key().ToString());
				}else{
					nextkey.clear();
				}

				if (it) delete it;
			}
			ar[0] = nextkey;
			ar[1] = nextseq;
			return cmdstatus::cs_data; 
		}
		// 读取binlog
		blog->read(seq,count,&ardata,&nextseq);
		ar[0] = nextkey;
		ar[1] = nextseq;
		return cmdstatus::cs_data;
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
		regist("del",&command_method_2_function<leveldb_store,&leveldb_store::on_del>,2,command_sepc::write|command_sepc::noscript,1,1,1);
		regist("cave_sync",command_method_2_function<leveldb_store,&leveldb_store::on_cave_sync>,4,command_sepc::skip_monitor|command_sepc::fast|command_sepc::noscript|command_sepc::readonly,0,0,0);
	}

	leveldb_store::~leveldb_store()
	{
		
	}

	void* leveldb_store::thread_auto_compact(leveldb::DB* ldb)
	{
		lyramilk::data::string cursor;
		lyramilk::data::string lastcur;
		const std::string lkey = ".sync.last_auto_compact";
		{
			leveldb::Status ldbs = ldb->Get(ropt,lkey,&cursor);

			if(ldbs.ok()){
				log(lyramilk::log::trace,__FUNCTION__) << D("自动整理进度:%s",cursor.c_str()) << std::endl;
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
								leveldb::Slice range_start = cursor;
								ldb->CompactRange(&range_start,nullptr);
								cursor.clear();
								skip_day = d;
								log(lyramilk::log::trace,__FUNCTION__) << D("自动整理完成，共整理%lld条",compact_total) << std::endl;
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
				if (lastcur != cursor){
					leveldb::Status ldbs = ldb->Put(wopt,lkey,cursor);
					if(ldbs.ok()){
						log(lyramilk::log::trace,__FUNCTION__) << D("写入 key:%s",cursor.c_str()) << std::endl;
					}else{
						log(lyramilk::log::error,__FUNCTION__) << D("加载key出错:%s",ldbs.ToString().c_str()) << std::endl;
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

		redis_pack pk;
		pk.type = redis_pack::s_native;
		pk.key = repkey;
		std::string lkey = redis_pack::pack(&pk);

		lyramilk::data::string stlsync_info((const char*)&offset,sizeof(lyramilk::data::uint64));
		stlsync_info.append(replid);

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
		lyramilk::data::string psync_replid;
		lyramilk::data::uint64 psync_offset;


	void leveldb_store::after_command(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen,const command_sepc& cmdspec,cmdstatus retcs)
	{
		if(!(cmdspec.flag&command_sepc::readonly)){
			if(blog) blog->append(args);
		}
	}





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
			log(lyramilk::log::warning,__FUNCTION__) << D("命令 hgetall %.*s 耗时%.3f",key.size(),key.c_str(),double(nsec) / 1000000) << std::endl;
		}

		*ret = result;
		return cmdstatus::cs_data;
	}

	cmdstatus leveldb_store::hget(const lyramilk::data::string& key,const lyramilk::data::string& field,lyramilk::data::string* ret) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();

		redis_pack pk;
		pk.type = redis_pack::s_hash;

		pk.key = key;
		pk.hash.field.assign(field.data(),field.size());

		std::string lkey = redis_pack::pack(&pk);

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

}}
