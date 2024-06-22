#include "binlog_store.h"

#include <libmilk/log.h>
#include <libmilk/testing.h>
#include <libmilk/dict.h>

#include <leveldb/db.h>
#include <leveldb/filter_policy.h>
#include <leveldb/cache.h>
#include <leveldb/write_batch.h>
#include <leveldb/comparator.h>
#include <leveldb/slice.h>

#include <endian.h>
#include <unistd.h>

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{

	static leveldb::ReadOptions ropt;
	static leveldb::WriteOptions wopt;
	lyramilk::log::logss static log(lyramilk::klog,"lyramilk.cave.binlog_leveldb");


	binlog_leveldb::binlog_leveldb()
	{
		ldb = nullptr;
		seq = 0;
		minseq = 0;
		maxseq = 0;
		capacity = 20000000;
		is_master = false;;
	}

	binlog_leveldb::~binlog_leveldb()
	{
		
	}

	void binlog_leveldb::set_master(bool is_master)
	{
		this->is_master = is_master;
	}

	void* binlog_leveldb::thread_clear_binlog(binlog_leveldb* blog)
	{
		while(true){
			if(blog->maxseq > blog->capacity){
				lyramilk::data::uint64 minseq = blog->maxseq - blog->capacity;
				lyramilk::data::string skey;
				skey.append("i+");

				long cc = 0;

				{
					leveldb::Iterator* it = blog->ldb->NewIterator(ropt);
					if(it != nullptr){
						it->Seek(skey);

						for(;it->Valid() && it->key().starts_with("i+");it->Next()){
							leveldb::Slice s = it->key();
							s.remove_prefix(2);
							if(s.size() == 8){
								lyramilk::data::uint64 currseq = be64toh(*(lyramilk::data::uint64*)s.data());
								if(currseq > minseq){
									break;
								}
								blog->ldb->Delete(wopt,it->key());
								cc++;
							}else{
								break;
							}
						}

						if (it) delete it;
					}


					lyramilk::data::uint64 tmp1 = htobe64(0);
					lyramilk::data::string skey1;
					skey1.append("i+");
					skey1.append((const char*)&tmp1,8);
					lyramilk::data::uint64 tmp2 = htobe64(minseq);
					lyramilk::data::string skey2;
					skey2.append("i+");
					skey2.append((const char*)&tmp2,8);

					leveldb::Slice s1 = skey1;
					leveldb::Slice s2 = skey2;

					blog->ldb->CompactRange(&s1,&s2);
					if(cc > 0){
						log(lyramilk::log::debug,__FUNCTION__) << "清理过期binlog条数:" << cc << ",capacity=" << blog->capacity << std::endl;
					}
				}
			}
			sleep(5);
		}
		return nullptr;
	}

	bool binlog_leveldb::open_leveldb(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing,long capacity)
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

			minseq = find_min();
			maxseq = find_max();
			seq = maxseq;
			log(lyramilk::log::debug,__FUNCTION__) << "cavedb.seq=" << minseq << "~" << maxseq << std::endl;

			this->capacity = capacity;
			log(lyramilk::log::debug,__FUNCTION__) << "binlog.capacity=" << this->capacity << std::endl;

			pthread_t thread;
			if(pthread_create(&thread,NULL,(void* (*)(void*))thread_clear_binlog,this) == 0){
				pthread_detach(thread);
			}
			return true;
		}
		delete ldb;
		ldb = nullptr;
		log(lyramilk::log::error,__FUNCTION__) << D("初始化leveldb失败%s",ldbs.ToString().c_str()) << std::endl;
		return false;
	}

	lyramilk::data::uint64 binlog_leveldb::find_min()
	{
		std::string sbinlog;

		lyramilk::data::string keep_prefix = "i+";
		{
			leveldb::Iterator* it = ldb->NewIterator(ropt);
			if(it != nullptr){
				it->Seek(keep_prefix);

				if(it->Valid() && it->key().starts_with(keep_prefix)){
					leveldb::Slice s = it->key();
					s.remove_prefix(2);
					sbinlog = s.ToString();
				}

				if (it) delete it;
			}
		}

		if(sbinlog.size() == 8){
			lyramilk::data::uint64 tmp = 0;
			sbinlog.copy((char*)&tmp,8);
			return be64toh(tmp);
		}
		return 0;
	}

	lyramilk::data::uint64 binlog_leveldb::find_max()
	{
		std::string sbinlog;
		lyramilk::data::string keep_prefix = "i+";
		lyramilk::data::string keep_prefix_eof = "i-";
		{
			leveldb::Iterator* it = ldb->NewIterator(ropt);
			if(it != nullptr){
				it->Seek(keep_prefix_eof);
				if(!it->Valid()){
					it->SeekToLast();
				}else{
					it->Prev();
				}

				if(it->Valid() && it->key().starts_with(keep_prefix)){
					leveldb::Slice s = it->key();
					s.remove_prefix(2);
					sbinlog = s.ToString();
				}

				if (it) delete it;
			}
		}

		if(sbinlog.size() == 8){
			lyramilk::data::uint64 tmp = 0;
			sbinlog.copy((char*)&tmp,8);
			return be64toh(tmp);
		}
		return 0;
	}

	bool binlog_leveldb::hset(const lyramilk::data::string& key,const lyramilk::data::string& field,const lyramilk::data::string& value)
	{
		if(!ldb) return false;
		if(!is_master) return false;
		maxseq = __sync_fetch_and_add(&seq,1);
		
		return hset_with_seq(seq,key,field,value);
	}

	bool binlog_leveldb::hdel(const lyramilk::data::string& key,const lyramilk::data::string& field)
	{
		if(!ldb) return false;
		if(!is_master) return false;
		maxseq = __sync_fetch_and_add(&seq,1);
		
		return hdel_with_seq(seq,key,field);
	}

	bool binlog_leveldb::sadd(const lyramilk::data::string& key,const lyramilk::data::string& member)
	{
		if(!ldb) return false;
		if(!is_master) return false;
		maxseq = __sync_fetch_and_add(&seq,1);
		
		return sadd_with_seq(seq,key,member);
	}

	bool binlog_leveldb::srem(const lyramilk::data::string& key,const lyramilk::data::string& member)
	{
		if(!ldb) return false;
		if(!is_master) return false;
		maxseq = __sync_fetch_and_add(&seq,1);
		
		return srem_with_seq(seq,key,member);
	}

	bool binlog_leveldb::zadd(const lyramilk::data::string& key,double score,const lyramilk::data::string& value)
	{
		if(!ldb) return false;
		if(!is_master) return false;
		maxseq = __sync_fetch_and_add(&seq,1);
		
		return zadd_with_seq(seq,key,score,value);
	}

	bool binlog_leveldb::zrem(const lyramilk::data::string& key,const lyramilk::data::string& value)
	{
		if(!ldb) return false;
		if(!is_master) return false;
		/*
		maxseq = seq;
		__sync_fetch_and_add(&seq,1);*/
		maxseq = __sync_fetch_and_add(&seq,1);
		
		return zrem_with_seq(seq,key,value);
	}


	bool binlog_leveldb::hset_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,const lyramilk::data::string& field,const lyramilk::data::string& value)
	{
		lyramilk::data::uint64 tmp = htobe64(seq);
		lyramilk::data::string skey;
		skey.append("i+");
		skey.append((const char*)&tmp,8);

		lyramilk::data::ostringstream ss;
		lyramilk::data::var v;
		v.type(lyramilk::data::var::t_array);
		lyramilk::data::array& ar = v;
		ar.reserve(4);
		ar.emplace_back("binlog_hset");
		ar.emplace_back(key);
		ar.emplace_back(field);
		ar.emplace_back(value);
		v.serialize(ss);
		lyramilk::data::string str = ss.str();

		leveldb::Status ldbs = ldb->Put(wopt,skey,str);
		if(ldbs.ok()){
			return true;
		}
		return false;
	}

	bool binlog_leveldb::hdel_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,const lyramilk::data::string& field)
	{
		lyramilk::data::uint64 tmp = htobe64(seq);
		lyramilk::data::string skey;
		skey.append("i+");
		skey.append((const char*)&tmp,8);

		lyramilk::data::ostringstream ss;
		lyramilk::data::var v;
		v.type(lyramilk::data::var::t_array);
		lyramilk::data::array& ar = v;
		ar.reserve(3);
		ar.emplace_back("binlog_hdel");
		ar.emplace_back(key);
		ar.emplace_back(field);
		v.serialize(ss);
		lyramilk::data::string str = ss.str();

		leveldb::Status ldbs = ldb->Put(wopt,skey,str);
		if(ldbs.ok()){
			return true;
		}
		return false;
	}

	bool binlog_leveldb::sadd_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,const lyramilk::data::string& member)
	{
		lyramilk::data::uint64 tmp = htobe64(seq);
		lyramilk::data::string skey;
		skey.append("i+");
		skey.append((const char*)&tmp,8);

		lyramilk::data::ostringstream ss;
		lyramilk::data::var v;
		v.type(lyramilk::data::var::t_array);
		lyramilk::data::array& ar = v;
		ar.reserve(3);
		ar.emplace_back("binlog_sadd");
		ar.emplace_back(key);
		ar.emplace_back(member);
		v.serialize(ss);
		lyramilk::data::string str = ss.str();

		leveldb::Status ldbs = ldb->Put(wopt,skey,str);
		if(ldbs.ok()){
			return true;
		}
		return false;
	}

	bool binlog_leveldb::srem_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,const lyramilk::data::string& member)
	{
		lyramilk::data::uint64 tmp = htobe64(seq);
		lyramilk::data::string skey;
		skey.append("i+");
		skey.append((const char*)&tmp,8);

		lyramilk::data::ostringstream ss;
		lyramilk::data::var v;
		v.type(lyramilk::data::var::t_array);
		lyramilk::data::array& ar = v;
		ar.reserve(3);
		ar.emplace_back("binlog_srem");
		ar.emplace_back(key);
		ar.emplace_back(member);
		v.serialize(ss);
		lyramilk::data::string str = ss.str();

		leveldb::Status ldbs = ldb->Put(wopt,skey,str);
		if(ldbs.ok()){
			return true;
		}
		return false;
	}

	bool binlog_leveldb::zadd_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,double score,const lyramilk::data::string& value)
	{
		lyramilk::data::uint64 tmp = htobe64(seq);
		lyramilk::data::string skey;
		skey.append("i+");
		skey.append((const char*)&tmp,8);

		lyramilk::data::ostringstream ss;
		lyramilk::data::var v;
		v.type(lyramilk::data::var::t_array);
		lyramilk::data::array& ar = v;
		ar.reserve(4);
		ar.emplace_back("binlog_zadd");
		ar.emplace_back(key);
		ar.emplace_back(score);
		ar.emplace_back(value);
		v.serialize(ss);
		lyramilk::data::string str = ss.str();

		leveldb::Status ldbs = ldb->Put(wopt,skey,str);
		if(ldbs.ok()){
			return true;
		}
		return false;
	}

	bool binlog_leveldb::zrem_with_seq(lyramilk::data::uint64 seq,const lyramilk::data::string& key,const lyramilk::data::string& value)
	{
		lyramilk::data::uint64 tmp = htobe64(seq);
		lyramilk::data::string skey;
		skey.append("i+");
		skey.append((const char*)&tmp,8);

		lyramilk::data::ostringstream ss;
		lyramilk::data::var v;
		v.type(lyramilk::data::var::t_array);
		lyramilk::data::array& ar = v;
		ar.reserve(3);
		ar.emplace_back("binlog_zrem");
		ar.emplace_back(key);
		ar.emplace_back(value);
		v.serialize(ss);
		lyramilk::data::string str = ss.str();

		leveldb::Status ldbs = ldb->Put(wopt,skey,str);
		if(ldbs.ok()){
			return true;
		}
		return false;
	}

	bool binlog_leveldb::read(lyramilk::data::uint64 seq,lyramilk::data::uint64 count,lyramilk::data::array* data,lyramilk::data::uint64* nextseq,bool withseq)
	{
		lyramilk::data::uint64 tmp = htobe64(seq);
		lyramilk::data::string skey;
		skey.append("i+");
		skey.append((const char*)&tmp,8);
		*nextseq = seq;


		{
			leveldb::Iterator* it = ldb->NewIterator(ropt);
			if(it != nullptr){
				it->Seek(skey);

				if(it->Valid()){
					data->reserve(count);
				}
				lyramilk::data::uint64 i = 0;

				for(;it->Valid() && i < count && it->key().starts_with("i+");it->Next(),++i){

					leveldb::Slice s = it->key();
					s.remove_prefix(2);

					if(s.size() == 8){
						lyramilk::data::uint64 dataseq = be64toh(*(lyramilk::data::uint64*)s.data());
						*nextseq = dataseq + 1;
						lyramilk::data::var v2;
						lyramilk::data::istringstream iss(it->value().ToString());
						v2.deserialize(iss);
						if(v2.type() == lyramilk::data::var::t_array){
							if(withseq){
								lyramilk::data::array& v2ar = v2;
								v2ar.push_back(dataseq);
							}else{
								lyramilk::data::array& v2ar = v2;
								lyramilk::data::string cmdstr = v2ar[0].str();
								if(cmdstr == "binlog_hset"){
									v2ar[0] = "hset";
								}else if(cmdstr == "binlog_hdel"){
									v2ar[0] = "hdel";
								}else if(cmdstr == "binlog_sadd"){
									v2ar[0] = "sadd";
								}else if(cmdstr == "binlog_srem"){
									v2ar[0] = "srem";
								}else if(cmdstr == "binlog_zadd"){
									v2ar[0] = "zadd";
								}else if(cmdstr == "binlog_zrem"){
									v2ar[0] = "zrem";
								}
							}
							data->push_back(v2);
						}
					}
				}

				if (it) delete it;
			}
		}
	}


	return true;

}}
