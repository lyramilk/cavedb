#include "cavedb.h"
#include "slave_redis.h"
#include "slave_ssdb.h"
#include "redis_to_leveldb.h"
#include "leveldb_util.h"
#include <libmilk/multilanguage.h>
#include <libmilk/testing.h>
#include <leveldb/db.h>
#include <leveldb/filter_policy.h>
#include <leveldb/cache.h>
#include <unistd.h>

namespace lyramilk{ namespace cave
{
	const static leveldb::Slice key_replid("repl:id");
	const static leveldb::Slice key_reploffset("repl:offset");

	database::database():log(lyramilk::klog,"lyramilk.cave.cavedb")
	{
		ldb = nullptr;
		redis_cmd_args = new redis_leveldb_handler;
		h = nullptr;
	}

	database::~database()
	{
		if(redis_cmd_args) delete redis_cmd_args;
		if(h) delete h;
		if(ldb) delete ldb;
	}

	static redis_leveldb_comparator cmr;

	bool database::init_leveldb(const lyramilk::data::string& leveldbpath)
	{
		leveldb::Options opt;

		int cache_size = 500;		//MB	16
		int block_size = 32;		//KB	16
		int write_buffer_size = 64;	//MB	16
		int max_open_files = cache_size / 1024 * 300;
		if(max_open_files < 500){
			max_open_files = 500;
		}
		if(max_open_files > 1000){
			max_open_files = 1000;
		}

		opt.create_if_missing = true;
		opt.max_open_files = max_open_files;
		opt.filter_policy = leveldb::NewBloomFilterPolicy(10);
		opt.block_cache = leveldb::NewLRUCache(cache_size * 1024 * 1024);
		opt.block_size = block_size * 1024;
		opt.write_buffer_size = write_buffer_size * 1024 * 1024;
		opt.max_file_size = 32 * 1024 * 1024;
		opt.compression = leveldb::kSnappyCompression;


		opt.comparator = &cmr;

		log(lyramilk::log::debug,__FUNCTION__) << "leveldb.max_open_files=" << opt.max_open_files << std::endl;
		log(lyramilk::log::debug,__FUNCTION__) << "leveldb.block_size=" << opt.block_size << std::endl;
		log(lyramilk::log::debug,__FUNCTION__) << "leveldb.write_buffer_size=" << opt.write_buffer_size << std::endl;
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
		log(lyramilk::log::debug,__FUNCTION__) << D("cfver=%.*s",redis_leveldb_handler::cfver.size(),redis_leveldb_handler::cfver.c_str()) << std::endl;
		if(ldbs.ok() || ldbs.IsNotFound()){
			if(formatseq != redis_leveldb_handler::cfver){
				delete ldb;
				ldb = nullptr;
				log(lyramilk::log::warning,__FUNCTION__) << D("leveldb需要重新初始化[%.*s]与[%.*s]不匹配",formatseq.size(),formatseq.c_str(),redis_leveldb_handler::cfver.size(),redis_leveldb_handler::cfver.c_str()) << std::endl;
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
				ldbs = ldb->Put(wopt,".cfver",redis_leveldb_handler::cfver);
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

		redis_cmd_args->init(ldb);
		redis_cmd_args->userdata = this;
		initpid = getpid();
		this->leveldbpath = leveldbpath;
		return true;
	}

	bool database::slaveof_redis(const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& pwd)
	{
		if(initpid != getpid()){
			if(redis_cmd_args) delete redis_cmd_args;
			redis_cmd_args = new redis_leveldb_handler;
			if(ldb) delete ldb;
			ldb = nullptr;
			if(!init_leveldb(leveldbpath)){
				log(lyramilk::log::error,__FUNCTION__) << D("禁止在fork/daemon之前初始化leveldb") << std::endl;
				return false;
			}
			log(lyramilk::log::warning,__FUNCTION__) << D("leveldb需要重新初始化，因为leveldb不是当前进程初始化的。") << std::endl;
		}

		if(ldb == nullptr){
			log(lyramilk::log::error,__FUNCTION__) << D("leveldb未初始化") << std::endl;
			return false;
		}
		lyramilk::data::string psync_replid = "?";
		lyramilk::data::uint64 psync_offset = 0;

		leveldb::ReadOptions ropt;

		std::string replid;
		leveldb::Status ldbs = ldb->Get(ropt,key_replid,&replid);
		if(ldbs.ok()){
			psync_replid = lyramilk::data::str(replid);
		}

		std::string reploffset;
		ldbs = ldb->Get(ropt,key_reploffset,&reploffset);
		if(ldbs.ok()){
			lyramilk::data::uint64 offset = redis_leveldb_handler::bytes2integer(reploffset);
			psync_offset = offset;
		}

		log(lyramilk::log::trace,__FUNCTION__) << D("载入:masterid=%s,offset=%llu",psync_replid.c_str(),psync_offset) << std::endl;
		slave_redis* r = new slave_redis;
		r->slaveof(host,port,pwd,psync_replid,psync_offset,this);
		h = r;
		return true;
	}

	bool database::slaveof_ssdb(const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& pwd)
	{
		if(initpid != getpid()){
			if(redis_cmd_args) delete redis_cmd_args;
			redis_cmd_args = new redis_leveldb_handler;
			if(ldb) delete ldb;
			ldb = nullptr;
			if(!init_leveldb(leveldbpath)){
				log(lyramilk::log::error,__FUNCTION__) << D("禁止在fork/daemon之前初始化leveldb") << std::endl;
				return false;
			}
			log(lyramilk::log::warning,__FUNCTION__) << D("leveldb需要重新初始化，因为leveldb不是当前进程初始化的。") << std::endl;
		}

		if(ldb == nullptr){
			log(lyramilk::log::error,__FUNCTION__) << D("leveldb未初始化") << std::endl;
			return false;
		}
		lyramilk::data::string psync_replid = "";
		lyramilk::data::uint64 psync_offset = 0;

		leveldb::ReadOptions ropt;

		std::string replid;
		leveldb::Status ldbs = ldb->Get(ropt,key_replid,&replid);
		if(ldbs.ok()){
			psync_replid = lyramilk::data::str(replid);
		}

		std::string reploffset;
		ldbs = ldb->Get(ropt,key_reploffset,&reploffset);
		if(ldbs.ok()){
			lyramilk::data::uint64 offset = redis_leveldb_handler::bytes2integer(reploffset);
			psync_offset = offset;
		}
		log(lyramilk::log::trace,__FUNCTION__) << D("载入:last_key=%s,last_seq=%llu",slave_ssdb::hexmem(psync_replid.c_str(),psync_replid.size()).c_str(),psync_offset) << std::endl;
		slave_ssdb* r = new slave_ssdb;
		r->slaveof(host,port,pwd,psync_replid,psync_offset,this);
		h = r;
		return true;
	}

	bool database::slaveof_ssdb(const lyramilk::data::string& leveldbpath,const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& pwd)
	{
		if(init_leveldb(leveldbpath)){
			return slaveof_ssdb(host,port,pwd);
		}
		return false;
	}

	bool database::slaveof_redis(const lyramilk::data::string& leveldbpath,const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& pwd)
	{
		return init_leveldb(leveldbpath) && slaveof_redis(host,port,pwd);
	}

	void database::notify_command(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
		redis_command_handle* h = cavedb_redis_commands::instance()->get(args[0].str());
		if(h){
			lyramilk::debug::nsecdiff td;
			td.mark();
			leveldb::WriteBatch batch;
			if(offset > 0){
				batch.Put(key_reploffset,redis_leveldb_handler::integer2bytes(offset));
			}
			if(!replid.empty()){
				batch.Put(key_replid,leveldb::Slice(replid.c_str(),replid.size()));
			}

			h(*redis_cmd_args,batch,args);

			leveldb::WriteOptions wopt;
			leveldb::Status ldbs = ldb->Write(wopt,&batch);
			if(ldbs.ok()){
				double mseccost = double(td.diff()) / 1000000;

				if(mseccost > 20){
					log(lyramilk::log::error,__FUNCTION__) << D("完成:") << args << D("%.3f (msec)",mseccost) << std::endl;
				}else if(mseccost > 2){
					log(lyramilk::log::warning,__FUNCTION__) << D("完成:") << args << D("%.3f (msec)",mseccost) << std::endl;
				}else{
#ifdef _DEBUG
					//log(lyramilk::log::debug,__FUNCTION__) << D("完成:") << args << D("%.3f (msec) seq=%lld",mseccost,offset) << std::endl;
#endif
				}
			}else{
				log(lyramilk::log::error,__FUNCTION__) << replid << "," << offset << ":" << D("失败:") << args << D("原因：%s",ldbs.ToString().c_str()) << std::endl;
			}
		}else{
			log(lyramilk::log::warning,__FUNCTION__) << replid << "," << offset << ":" << D("未实现:") << args << std::endl;
		}
	}

	bool database::notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
	{
		leveldb::WriteOptions wopt;
		leveldb::Status ldbs = ldb->Put(wopt,key_replid,leveldb::Slice(replid.c_str(),replid.size()));
		if(!ldbs.ok()) return false;
		ldbs = ldb->Put(wopt,key_reploffset,redis_leveldb_handler::integer2bytes(offset));
		log(lyramilk::log::debug,__FUNCTION__) << "psync " << replid << "," << offset << std::endl;
		return ldbs.ok();
	}

	bool database::notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
	{
		time_t now = time(nullptr);

		if(lastcompat + 60 < now){
			lyramilk::debug::nsecdiff td;
			td.mark();
			ldb->CompactRange(nullptr,nullptr);
			double mseccost = double(td.diff()) / 1000000;

			if(mseccost > 2000){
				log(lyramilk::log::error,__FUNCTION__) << D("主库id:%s,同步偏移:%llu   cost=%.3f (mesc)",replid.c_str(),offset,mseccost) << std::endl;
			}else if(mseccost > 600){
				log(lyramilk::log::warning,__FUNCTION__) << D("主库id:%s,同步偏移:%llu   cost=%.3f (mesc)",replid.c_str(),offset,mseccost) << std::endl;
			}else{
				//log(lyramilk::log::debug,__FUNCTION__) << D("主库id:%s,同步偏移:%llu   cost=%.3f (mesc)",replid.c_str(),offset,mseccost) << std::endl;
			}
			lastcompat = now;
		}
		return true;
	}

	void database::dump(std::ostream& os)
	{
	
	
	}

}}
