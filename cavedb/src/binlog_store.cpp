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

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{

	extern leveldb::ReadOptions ropt;
	extern leveldb::WriteOptions wopt;
	lyramilk::log::logss static log(lyramilk::klog,"lyramilk.cave.binlog");

	bool binlog_reader::recv_callback(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args)
	{
		return true;
	}



	binlog::binlog()
	{
		ldb = nullptr;
		seq = 0;
	}

	binlog::~binlog()
	{
		
	}

	void* binlog::thread_clear_binlog(binlog* blog)
	{
		return nullptr;
	}

	bool binlog::open_leveldb(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing)
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

			seq = find_max();

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

	lyramilk::data::uint64 binlog::find_min()
	{
		std::string sbinlog;

		lyramilk::data::string keep_prefix = "i+";
		{
			leveldb::Iterator* it = ldb->NewIterator(ropt);
			if(it != nullptr){
				it->Seek(keep_prefix);

				if(it->Valid() && it->key().starts_with(keep_prefix)){
					leveldb::Slice s = it->value();
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

	lyramilk::data::uint64 binlog::find_max()
	{
		std::string sbinlog;
		lyramilk::data::string keep_prefix = "i-";
		{
			leveldb::Iterator* it = ldb->NewIterator(ropt);
			if(it != nullptr){
				it->Seek(keep_prefix);
				if(!it->Valid()){
					it->SeekToLast();
				}else{
					it->Prev();
				}

				if(it->Valid() && it->key().starts_with(keep_prefix)){
					leveldb::Slice s = it->value();
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

	bool binlog::append(const lyramilk::data::array& args)
	{
		if(!ldb) return false;
		lyramilk::data::uint64 tmp = htobe64(seq);
		lyramilk::data::string skey;
		skey.append("i+");
		skey.append((const char*)&tmp,8);

		lyramilk::data::ostringstream ss;
		lyramilk::data::var v(args);
		v.serialize(ss);
		lyramilk::data::string str = ss.str();

		leveldb::Status ldbs = ldb->Put(wopt,skey,str);
		return ldbs.ok();
	}

	void binlog::read(lyramilk::data::uint64 seq,binlog_reader* reader)
	{
		lyramilk::data::uint64 tmp = htobe64(seq);
		lyramilk::data::string skey;
		skey.append("i+");
		skey.append((const char*)&tmp,8);


		{
			leveldb::Iterator* it = ldb->NewIterator(ropt);
			if(it != nullptr){
				it->Seek(skey);

				if(it->Valid() && it->key().starts_with("i+")){
					leveldb::Slice s = it->key();
					s.remove_prefix(2);

					if(s.size() == 8){
						lyramilk::data::uint64 tmp = *(lyramilk::data::uint64*)s.data();

						lyramilk::data::var v2;
						lyramilk::data::istringstream iss(it->value().ToString());
						v2.deserialize(iss);
						lyramilk::data::array& args = v2;
						if(v2.type() == lyramilk::data::var::t_array){
							reader->recv_callback("",be64toh(tmp),args);
						}
					}
				}

				if (it) delete it;
			}
		}
	}




}}