#include "leveldb_minimal.h"
#include "slave_redis.h"
#include "slave_ssdb.h"
#include <libmilk/log.h>
#include <libmilk/multilanguage.h>

#include <leveldb/db.h>
#include <leveldb/filter_policy.h>
#include <leveldb/cache.h>
#include <leveldb/write_batch.h>

namespace lyramilk{ namespace cave
{
	lyramilk::log::logss static log(lyramilk::klog,"lyramilk.cave.store.leveldb_minimal");
	const std::string leveldb_minimal::cfver = "1_mininal";
	leveldb::ReadOptions ropt;
	leveldb::WriteOptions wopt;

	static const char* key_sync = ".sync.key";

	void inline save_process(leveldb::WriteBatch& batch,const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
	{
		lyramilk::data::string str((const char*)&offset,sizeof(lyramilk::data::uint64));
		str.append(replid);
		batch.Put(key_sync,leveldb::Slice(str.c_str(),str.size()));
	}

	bool leveldb_minimal::notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
	{
		leveldb::WriteBatch batch;
		save_process(batch,replid,offset);
		ldb->Write(wopt,&batch);
		return true;
	}

	bool leveldb_minimal::notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
	{
		leveldb::WriteBatch batch;
		save_process(batch,replid,offset);
		ldb->Write(wopt,&batch);
		return true;
	}

	void leveldb_minimal::notify_flushdb(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		leveldb::Iterator* it = ldb->NewIterator(ropt);
		if(it == nullptr){
			log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			return;
		}

		for(it->SeekToFirst();it->Valid();it->Next()){
			ldb->Delete(wopt,it->key());
		}
		if (it) delete it;

		ldb->CompactRange(nullptr,nullptr);
		ldb->Put(wopt,".cfver",cfver);
	}

	void leveldb_minimal::notify_flushall(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		leveldb::Iterator* it = ldb->NewIterator(ropt);
		if(it == nullptr){
			log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			return;
		}

		for(it->SeekToFirst();it->Valid();it->Next()){
			ldb->Delete(wopt,it->key());
		}
		if (it) delete it;

		ldb->CompactRange(nullptr,nullptr);
		ldb->Put(wopt,".cfver",cfver);
	}

	void leveldb_minimal::notify_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		leveldb::WriteBatch batch;
		save_process(batch,replid,offset);

		std::string prefix = args[1];
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		leveldb::Iterator* it = ldb->NewIterator(ropt);
		if(it == nullptr){
			log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			return;
		}

		for(it->Seek(prefix);it->Valid();it->Next()){
			if(!it->key().starts_with(prefix)) break;
			batch.Delete(it->key());
		}
		if (it) delete it;

		ldb->Write(wopt,&batch);
	}

	void leveldb_minimal::notify_move(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		log(lyramilk::log::error,__FUNCTION__) << D("未实现move函数，这在ssdb中不应该出现") << std::endl;
	}

	void leveldb_minimal::notify_pexpireat(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
	}

	void leveldb_minimal::notify_persist(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
	}

	void leveldb_minimal::notify_rename(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		log(lyramilk::log::error,__FUNCTION__) << D("未实现rename函数，这在ssdb中不应该出现") << std::endl;
	}

	void leveldb_minimal::notify_hset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		leveldb::WriteBatch batch;
		save_process(batch,replid,offset);
		std::string prefix = args[1];
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(args[2]);
		batch.Put(prefix,lyramilk::data::str(args[3].str()));
		ldb->Write(wopt,&batch);
	}

	void leveldb_minimal::notify_hdel(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		leveldb::WriteBatch batch;
		save_process(batch,replid,offset);
		std::string prefix = args[1];
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(args[2]);
		batch.Delete(prefix);
		ldb->Write(wopt,&batch);
	}

	leveldb_minimal::leveldb_minimal()
	{
		ldb = nullptr;
		sem.set_max_signal(10);
	}

	leveldb_minimal::~leveldb_minimal()
	{
		delete ldb;
	}

	bool leveldb_minimal::open(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB)
	{
		leveldb::Options opt;

		int block_size = 32;		//KB	16
		int write_buffer_size = 64;	//MB	16
		int max_open_files = cache_size_MB / 1024 * 300;
		if(max_open_files < 500){
			max_open_files = 500;
		}
		if(max_open_files > 1000){
			max_open_files = 1000;
		}

		opt.create_if_missing = true;
		opt.max_open_files = max_open_files;
		opt.filter_policy = leveldb::NewBloomFilterPolicy(10);
		opt.block_cache = leveldb::NewLRUCache(cache_size_MB * 1024 * 1024);
		opt.block_size = block_size * 1024;
		opt.write_buffer_size = write_buffer_size * 1024 * 1024;
		opt.max_file_size = 32 * 1024 * 1024;
		opt.compression = leveldb::kSnappyCompression;

		log(lyramilk::log::debug,__FUNCTION__) << "leveldb.max_open_files=" << opt.max_open_files << std::endl;
		log(lyramilk::log::debug,__FUNCTION__) << "leveldb.block_size=" << opt.block_size << std::endl;
		log(lyramilk::log::debug,__FUNCTION__) << "leveldb.write_buffer_size=" << opt.write_buffer_size << std::endl;
		log(lyramilk::log::debug,__FUNCTION__) << "leveldb.max_file_size=" << opt.max_file_size << std::endl;
		log(lyramilk::log::debug,__FUNCTION__) << "leveldb.compression=" << opt.compression << std::endl;

		leveldb::Status ldbs = leveldb::DB::Open(opt,leveldbpath.c_str(),&ldb);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("初始化leveldb失败%s",ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		if(ldb == nullptr){
			log(lyramilk::log::error,__FUNCTION__) << D("初始化leveldb失败") << std::endl;
			return false;
		}

		leveldb::ReadOptions ropt;
		std::string formatseq;
		ldbs = ldb->Get(ropt,".cfver",&formatseq);
		log(lyramilk::log::debug,__FUNCTION__) << D("cfver=%.*s",cfver.size(),cfver.c_str()) << std::endl;
		if(ldbs.ok() || ldbs.IsNotFound()){
			if(formatseq != cfver){
				delete ldb;
				ldb = nullptr;
				log(lyramilk::log::warning,__FUNCTION__) << D("leveldb需要重新初始化[%.*s]与[%.*s]不匹配",formatseq.size(),formatseq.c_str(),cfver.size(),cfver.c_str()) << std::endl;
				leveldb::Status ldbs = DestroyDB(leveldbpath.c_str(),opt);
				if(!ldbs.ok()){
					log(lyramilk::log::error,__FUNCTION__) << D("初始化leveldb失败%s",ldbs.ToString().c_str()) << std::endl;
					return false;
				}
				ldbs = leveldb::DB::Open(opt,leveldbpath.c_str(),&ldb);
				if(!ldbs.ok()){
					log(lyramilk::log::error,__FUNCTION__) << D("初始化leveldb失败%s",ldbs.ToString().c_str()) << std::endl;
					return false;
				}
				if(ldb == nullptr){
					log(lyramilk::log::error,__FUNCTION__) << D("初始化leveldb失败") << std::endl;
					return false;
				}

				leveldb::WriteOptions wopt;
				ldbs = ldb->Put(wopt,".cfver",cfver);
				if(!ldbs.ok()){
					log(lyramilk::log::error,__FUNCTION__) << D("初始化leveldb失败%s",ldbs.ToString().c_str()) << std::endl;
					return false;
				}
			}
		}else{
			delete ldb;
			ldb = nullptr;
			log(lyramilk::log::error,__FUNCTION__) << D("初始化leveldb失败%s",ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}

	bool leveldb_minimal::get_sync_info(lyramilk::data::string* replid,lyramilk::data::uint64* offset) const
	{
		if(replid == nullptr || offset == nullptr) return false;
		std::string stlsync_info;
		leveldb::Status ldbs = ldb->Get(ropt,key_sync,&stlsync_info);
		if(!ldbs.ok()){
			return false;
		}
		*replid = lyramilk::data::str(stlsync_info.substr(sizeof(lyramilk::data::uint64)));
		stlsync_info.copy((char*)offset,sizeof(lyramilk::data::uint64));
		return true;
	}

	bool leveldb_minimal::compact()
	{
		ldb->CompactRange(nullptr,nullptr);
		return true;
	}

	bool leveldb_minimal::hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		rspeed_on_read();

		std::string prefix;
		prefix.reserve(key.size() + field.size() + 2 + 2);
		prefix.append(key.c_str(),key.size());
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(field.c_str(),field.size());

		std::string result;
		leveldb::Status ldbs = ldb->Get(ropt,prefix,&result);
		return ldbs.ok();
	}

	lyramilk::data::string leveldb_minimal::hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		rspeed_on_read();

		std::string prefix;
		prefix.reserve(key.size() + field.size() + 2 + 2);
		prefix.append(key.c_str(),key.size());
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(field.c_str(),field.size());

		std::string result;
		{
			lyramilk::threading::mutex_sync _(sem);
			leveldb::Status ldbs = ldb->Get(ropt,prefix,&result);
			if(!ldbs.ok()){
				return "";
			}
		}
		return lyramilk::data::str(result);
	}

	lyramilk::data::stringdict leveldb_minimal::hgetall(const lyramilk::data::string& key) const
	{
		rspeed_on_read();

		std::string prefix;
		prefix.reserve(key.size() + 2 + 2);
		prefix = key;
		prefix.push_back(0xff);
		prefix.push_back(0xfe);


		lyramilk::data::stringdict result;
		leveldb::Iterator* it = nullptr;
		{
			lyramilk::threading::mutex_sync _(sem);
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
				return result;
			}

			for(it->Seek(prefix);it->Valid();it->Next()){
				if(!it->key().starts_with(prefix)) break;

				leveldb::Slice datakey = it->key();
				datakey.remove_prefix(prefix.size());

				lyramilk::data::string skey(datakey.data(),datakey.size());
				lyramilk::data::string svalue(it->value().data(),it->value().size());
				result[skey] = svalue;
			}

			if (it) delete it;
		}
		return result;
	}


	long long leveldb_minimal::get_sigval()
	{
		return sem.get_signal();
	}
}}
