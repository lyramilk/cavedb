#include "leveldb_minimal2.h"
#include "slave_redis.h"
#include "slave_ssdb.h"
#include <libmilk/log.h>
#include <libmilk/dict.h>
#include <libmilk/testing.h>

#include <leveldb/db.h>
#include <leveldb/filter_policy.h>
#include <leveldb/cache.h>
#include <leveldb/write_batch.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

namespace lyramilk{ namespace cave
{
	lyramilk::log::logss static log(lyramilk::klog,"lyramilk.cave.store.leveldb_minimal2");
	const std::string leveldb_minimal2::cfver = "2_mininal";
	extern leveldb::ReadOptions ropt;
	extern leveldb::WriteOptions wopt;

	void inline save_process(leveldb::WriteBatch& batch,const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
	{
		if(offset == 0 && replid.empty()) return;

		lyramilk::data::string repkey;
		if(masterid.empty()){
			repkey = ".sync.key";
		}else{
			repkey = ".sync.key.";
			repkey += masterid;
		}

		lyramilk::data::string str((const char*)&offset,sizeof(lyramilk::data::uint64));
		str.append(replid);
		batch.Put(repkey,leveldb::Slice(str.c_str(),str.size()));
	}

	bool leveldb_minimal2::notify_idle(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata)
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

	bool leveldb_minimal2::notify_psync(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata)
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

	bool leveldb_minimal2::notify_flushdb(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
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

	bool leveldb_minimal2::notify_flushall(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
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

	bool leveldb_minimal2::notify_del(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);

		for(char c = 0xfa;c<0xff;++c){
			std::string prefix;
			prefix.push_back(0xff);
			prefix.push_back(c);
			prefix.append(args[1]);

			leveldb::Iterator* it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
				return false;
			}

			for(it->Seek(prefix);it->Valid();it->Next()){
				if(!it->key().starts_with(prefix)) break;
				batch.Delete(it->key());
			}
			if (it) delete it;

			leveldb::Status ldbs = ldb->Write(wopt,&batch);
			if(!ldbs.ok()){
				log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
				return false;
			}
		}
		return true;
	}

	bool leveldb_minimal2::notify_move(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		log(lyramilk::log::error,__FUNCTION__) << D("未实现move函数，这在ssdb中不应该出现") << std::endl;
		return false;
	}

	bool leveldb_minimal2::notify_pexpireat(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return false;
	}

	bool leveldb_minimal2::notify_persist(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return false;
	}

	bool leveldb_minimal2::notify_rename(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		log(lyramilk::log::error,__FUNCTION__) << D("未实现rename函数，这在ssdb中不应该出现") << std::endl;
		return false;
	}

	bool leveldb_minimal2::notify_hset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);
		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(args[1]);
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(args[2]);

		batch.Put(prefix,lyramilk::data::str(args[3].str()));
		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}

	bool leveldb_minimal2::notify_hdel(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);

		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(args[1]);
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(args[2]);

		batch.Delete(prefix);
		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}


	bool leveldb_minimal2::notify_set(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);

		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfd);
		prefix.append(args[1]);

		batch.Put(prefix,lyramilk::data::str(args[2].str()));
		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}

	bool leveldb_minimal2::notify_ssdb_del(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);

		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfd);
		prefix.append(args[1]);

		batch.Delete(prefix);
		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}

	bool leveldb_minimal2::notify_ssdb_qset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);

		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfc);
		prefix.append(args[1]);
		prefix.push_back(0xff);
		prefix.push_back(0xfc);
		prefix.append(args[2]);

		batch.Put(prefix,lyramilk::data::str(args[3].str()));
		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}

	bool leveldb_minimal2::notify_lpop(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfc);
		prefix.append(args[1]);

		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
				return false;
			}

			it->Seek(prefix);
			if(!it->Valid()){
				log(lyramilk::log::error,__FUNCTION__) << D("迭代器错误:%s","Seek") << std::endl;
				return true;
			}
			if(it->key().starts_with(prefix)){
			}
				leveldb::WriteBatch batch;
				save_process(batch,masterid,replid,offset);
				batch.Delete(it->key());
				leveldb::Status ldbs = ldb->Write(wopt,&batch);
				if(!ldbs.ok()){
					log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
					return false;
				}
		}

		return true;
	}

	bool leveldb_minimal2::notify_rpop(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfc);
		prefix.append(args[1]);
		prefix.push_back(0xff);
		prefix.push_back(0xfc);

		std::string last;
		last.reserve(prefix.size());
		last.push_back(0xff);
		last.push_back(0xfc);
		last.append(args[1]);
		last.push_back(0xff);
		last.push_back(0xfd);

		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
				return false;
			}

			it->Seek(last);

			if(!it->Valid()){
				it->SeekToLast();
				if(!it->Valid()){
					log(lyramilk::log::error,__FUNCTION__) << D("迭代器错误:%s","SeekToLast") << std::endl;
					return true;
				}
			}else{
				it->Prev();
				if(!it->Valid()){
					log(lyramilk::log::error,__FUNCTION__) << D("迭代器错误:%s","Prev") << std::endl;
					return true;
				}
			}

			if(!it->key().starts_with(prefix)){
				log(lyramilk::log::warning,__FUNCTION__) << D("容器己空") << std::endl;
				return true;
			}
			leveldb::WriteBatch batch;
			save_process(batch,masterid,replid,offset);
			batch.Delete(it->key());
			leveldb::Status ldbs = ldb->Write(wopt,&batch);
			if(!ldbs.ok()){
				log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
				return false;
			}
		}
		return true;
	}

	bool leveldb_minimal2::notify_zadd(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);

		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfb);
		prefix.append(args[1]);
		prefix.push_back(0xff);
		prefix.push_back(0xfb);
		prefix.append(args[2]);

		batch.Delete(prefix);
		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}

	bool leveldb_minimal2::notify_zrem(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		leveldb::WriteBatch batch;
		save_process(batch,masterid,replid,offset);

		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfb);
		prefix.append(args[1]);
		prefix.push_back(0xff);
		prefix.push_back(0xfb);
		prefix.append(args[2]);

		batch.Delete(prefix);
		leveldb::Status ldbs = ldb->Write(wopt,&batch);
		if(!ldbs.ok()){
			log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
			return false;
		}
		return true;
	}

	leveldb_minimal2::leveldb_minimal2()
	{
		ldb = nullptr;
	}

	leveldb_minimal2::~leveldb_minimal2()
	{
		delete ldb;
	}

	minimal_interface* leveldb_minimal2::open(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing = false)
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
						if(fd_flag == -1)  return nullptr;
						int w = write(fd_flag,cfver.c_str(),cfver.size());
						::close(fd_flag);
						if(w == (int)cfver.size()){
							flag = cfver;
						}
					}else{
						return nullptr;
					}
				}else{
					return nullptr;
				}
			}else{
				int fd_flag = ::open(fflag.c_str(),O_RDONLY,0444);
				if(fd_flag == -1)  return nullptr;
				char buff[1024];
				int r = read(fd_flag,buff,sizeof(buff));
				::close(fd_flag);
				if(r > 0){
					flag.assign(buff,r);
				}
			}
		}

		if(flag != cfver) return nullptr;


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
			return nullptr;
		}
		if(!ldbs.ok()){
			delete ldb;
			ldb = nullptr;
			log(lyramilk::log::error,__FUNCTION__) << D("初始化leveldb失败%s",ldbs.ToString().c_str()) << std::endl;
			return nullptr;
		}

		leveldb::ReadOptions ropt;
		std::string formatseq;
		ldbs = ldb->Get(ropt,".cfver",&formatseq);
		if(ldbs.ok() && formatseq != cfver){
			delete ldb;
			ldb = nullptr;
			log(lyramilk::log::warning,__FUNCTION__) << D("leveldb打开失败：[%.*s]与[%.*s]不匹配",formatseq.size(),formatseq.c_str(),cfver.size(),cfver.c_str()) << std::endl;
			return nullptr;
		}else if(ldbs.IsNotFound()){
			leveldb::WriteBatch batch;
			batch.Put(".cfver",cfver);
			ldbs = ldb->Write(wopt,&batch);
		}

		if(ldbs.ok() || ldbs.IsNotFound()){
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.cfver=" << formatseq << std::endl;
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.max_open_files=" << opt.max_open_files << std::endl;
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.block_size=" << opt.block_size << std::endl;
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.write_buffer_size=" << opt.write_buffer_size << std::endl;
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.max_file_size=" << opt.max_file_size << std::endl;
			log(lyramilk::log::debug,__FUNCTION__) << "leveldb.compression=" << opt.compression << std::endl;
			log(lyramilk::log::debug,__FUNCTION__) << D("cfver=%.*s",cfver.size(),cfver.c_str()) << std::endl;
			leveldb_minimal2* ins = new leveldb_minimal2();
			ins->ldb = ldb;
			return ins;
		}
		delete ldb;
		ldb = nullptr;
		log(lyramilk::log::error,__FUNCTION__) << D("初始化leveldb失败%s",ldbs.ToString().c_str()) << std::endl;
		return nullptr;
	}

	bool leveldb_minimal2::get_sync_info(const lyramilk::data::string& masterid,lyramilk::data::string* replid,lyramilk::data::uint64* offset) const
	{
		if(replid == nullptr || offset == nullptr) return false;

		lyramilk::data::string repkey;
		if(masterid.empty()){
			repkey = ".sync.key";
		}else{
			repkey = ".sync.key.";
			repkey += masterid;
		}

		std::string stlsync_info;
		leveldb::Status ldbs = ldb->Get(ropt,repkey,&stlsync_info);
		if(!ldbs.ok()){
			if(ldbs.IsNotFound()){
				replid->clear();
				*offset = 0;
				return true;
			}

			log(lyramilk::log::error,__FUNCTION__) << D("获取同步位置失败 %s",ldbs.ToString().c_str()) << std::endl;
			return false;
		}

		*replid = lyramilk::data::str(stlsync_info.substr(sizeof(lyramilk::data::uint64)));
		stlsync_info.copy((char*)offset,sizeof(lyramilk::data::uint64));
		return true;
	}

	bool leveldb_minimal2::compact()
	{
		ldb->CompactRange(nullptr,nullptr);
		return true;
	}

	bool leveldb_minimal2::hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		rspeed_on_read();

		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(key);
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(field);

		std::string result;
		leveldb::Status ldbs = ldb->Get(ropt,prefix,&result);

		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,"hexist") << D("命令 hexist %.*s,%.*s 耗时%.3f",key.size(),key.c_str(),field.size(),field.c_str(),double(nsec) / 1000000) << std::endl;
		}
		return ldbs.ok();
	}

	lyramilk::data::string leveldb_minimal2::hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		rspeed_on_read();

		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(key);
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(field);

		std::string result;
		{
			leveldb::Status ldbs = ldb->Get(ropt,prefix,&result);
			if(!ldbs.ok()){
				result.clear();
			}
		}
		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,"hget") << D("命令 hget %.*s,%.*s 耗时%.3f",key.size(),key.c_str(),field.size(),field.c_str(),double(nsec) / 1000000) << std::endl;
		}
		return lyramilk::data::str(result);
	}

	lyramilk::data::stringdict leveldb_minimal2::hgetall(const lyramilk::data::string& key) const
	{
		lyramilk::debug::nsecdiff nd;
		nd.mark();
		rspeed_on_read();

		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(key);
		prefix.push_back(0xff);
		prefix.push_back(0xfe);

		lyramilk::data::stringdict result;
		leveldb::Iterator* it = nullptr;
		{
			it = ldb->NewIterator(ropt);
			if(it == nullptr){
				log(lyramilk::log::error,__FUNCTION__) << D("创建迭代器失败") << std::endl;
			}else{
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
		}
		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,"hgetall") << D("命令 hgetall %.*s 耗时%.3f",key.size(),key.c_str(),double(nsec) / 1000000) << std::endl;
		}
		return result;
	}

	lyramilk::data::string leveldb_minimal2::get_property(const lyramilk::data::string& property)
	{
		std::string result;
		if(ldb->GetProperty(property,&result)){
			return lyramilk::data::str(result);
		}
		return "";
	}
}}
