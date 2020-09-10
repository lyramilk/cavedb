#include "lmdb_minimal.h"
#include "slave_redis.h"
#include "slave_ssdb.h"
#include <libmilk/log.h>
#include <libmilk/dict.h>
#include <libmilk/testing.h>
#include <string.h>



#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

namespace lyramilk{ namespace cave
{
	lyramilk::log::logss static log(lyramilk::klog,"lyramilk.cave.store.lmdb_minimal");
	const std::string lmdb_minimal::cfver = "2_lmdb_mininal";

	void inline save_process(MDB_txn* txn,MDB_dbi dbi,const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
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
		MDB_val key, data;
		key.mv_size = repkey.size();
		key.mv_data = (char*)repkey.c_str();
		data.mv_size = str.size();
		data.mv_data = (char*)str.c_str();

		mdb_put(txn, dbi, &key, &data, 0);
	}


	bool lmdb_minimal::notify_idle(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata)
	{
		MDB_txn* txn;
		mdb_txn_begin(env, nullptr, 0, &txn);
		save_process(txn,dbi,replid,offset);
		mdb_txn_commit(txn);
		return true;
	}

	bool lmdb_minimal::notify_psync(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata)
	{
		MDB_txn* txn;
		mdb_txn_begin(env, nullptr, 0, &txn);
		save_process(txn,dbi,replid,offset);
		mdb_txn_commit(txn);
		return true;
	}

	bool lmdb_minimal::notify_flushdb(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		MDB_txn* txn;
		mdb_txn_begin(env, nullptr, 0, &txn);
		save_process(txn,dbi,replid,offset);
		mdb_drop(txn,dbi,0);
		mdb_txn_commit(txn);
		return true;
	}

	bool lmdb_minimal::notify_flushall(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		MDB_txn* txn;
		mdb_txn_begin(env, nullptr, 0, &txn);
		save_process(txn,dbi,replid,offset);
		mdb_drop(txn,dbi,0);
		mdb_txn_commit(txn);
		return true;
	}

	bool lmdb_minimal::notify_del(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		TODO();
		/*
		leveldb::WriteBatch batch;
		save_process(batch,replid,offset);

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
		return true;*/
	}

	bool lmdb_minimal::notify_move(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		log(lyramilk::log::error,__FUNCTION__) << D("未实现move函数，这在ssdb中不应该出现") << std::endl;
		return false;
	}

	bool lmdb_minimal::notify_pexpireat(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return false;
	}

	bool lmdb_minimal::notify_persist(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return false;
	}

	bool lmdb_minimal::notify_rename(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		log(lyramilk::log::error,__FUNCTION__) << D("未实现rename函数，这在ssdb中不应该出现") << std::endl;
		return false;
	}

	bool lmdb_minimal::notify_hset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		MDB_txn* txn;
		mdb_txn_begin(env, nullptr, 0, &txn);
		save_process(txn,dbi,replid,offset);

		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(args[1]);
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(args[2]);

		std::string value = lyramilk::data::str(args[3].str());

		MDB_val key, data;
		key.mv_size = prefix.size();
		key.mv_data = (char*)prefix.c_str();
		data.mv_size = value.size();
		data.mv_data = (char*)value.c_str();

		mdb_put(txn, dbi, &key, &data, 0);
		mdb_txn_commit(txn);
		return true;
	}

	bool lmdb_minimal::notify_hdel(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		MDB_txn* txn;
		mdb_txn_begin(env, nullptr, 0, &txn);
		save_process(txn,dbi,replid,offset);

		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(args[1]);
		prefix.push_back(0xff);
		prefix.push_back(0xfe);
		prefix.append(args[2]);


		MDB_val key, data;
		key.mv_size = prefix.size();
		key.mv_data = (char*)prefix.c_str();

		mdb_del(txn, dbi, &key, nullptr);
		mdb_txn_commit(txn);
		return true;
	}


	bool lmdb_minimal::notify_set(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		MDB_txn* txn;
		mdb_txn_begin(env, nullptr, 0, &txn);
		save_process(txn,dbi,replid,offset);

		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfd);
		prefix.append(args[1]);

		std::string value = lyramilk::data::str(args[2].str());

		MDB_val key, data;
		key.mv_size = prefix.size();
		key.mv_data = (char*)prefix.c_str();
		data.mv_size = value.size();
		data.mv_data = (char*)value.c_str();

		mdb_put(txn, dbi, &key, &data, 0);
		mdb_txn_commit(txn);
		return true;
	}

	bool lmdb_minimal::notify_ssdb_del(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		MDB_txn* txn;
		mdb_txn_begin(env, nullptr, 0, &txn);
		save_process(txn,dbi,replid,offset);

		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfd);
		prefix.append(args[1]);

		MDB_val key;
		key.mv_size = prefix.size();
		key.mv_data = (char*)prefix.c_str();

		mdb_del(txn, dbi, &key, nullptr);
		mdb_txn_commit(txn);
		return true;
	}

	bool lmdb_minimal::notify_ssdb_qset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		TODO();
		/*
		leveldb::WriteBatch batch;
		save_process(batch,replid,offset);

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
		return true;*/
	}

	bool lmdb_minimal::notify_lpop(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		TODO();
		/*
		std::string prefix;
		prefix.reserve(256);
		prefix.push_back(0xff);
		prefix.push_back(0xfc);
		prefix.append(args[1]);

		leveldb::Iterator* it = nullptr;
		{
			lyramilk::threading::mutex_sync _(sem);
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
				save_process(batch,replid,offset);
				batch.Delete(it->key());
				leveldb::Status ldbs = ldb->Write(wopt,&batch);
				if(!ldbs.ok()){
					log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
					return false;
				}
		}

		return true;*/
	}

	bool lmdb_minimal::notify_rpop(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		TODO();
		/*
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
			lyramilk::threading::mutex_sync _(sem);
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
			save_process(batch,replid,offset);
			batch.Delete(it->key());
			leveldb::Status ldbs = ldb->Write(wopt,&batch);
			if(!ldbs.ok()){
				log(lyramilk::log::error,__FUNCTION__) << D("%s错误：%s\n",__FUNCTION__,ldbs.ToString().c_str()) << std::endl;
				return false;
			}
		}
		return true;*/
	}

	bool lmdb_minimal::notify_zadd(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		TODO();
		/*
		leveldb::WriteBatch batch;
		save_process(batch,replid,offset);

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
		return true;*/
	}

	bool lmdb_minimal::notify_zrem(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		TODO();
		/*
		leveldb::WriteBatch batch;
		save_process(batch,replid,offset);

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
		return true;*/
	}

	lmdb_minimal::lmdb_minimal()
	{
	}

	lmdb_minimal::~lmdb_minimal()
	{
	}

	minimal_interface* lmdb_minimal::open(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing = false)
	{
		lyramilk::data::string flag;
		{
			lyramilk::data::string fflag = leveldbpath + "/cavedb.flag";
			struct stat st = {0};
			if(0 !=::stat(fflag.c_str(),&st)){
				if(errno == ENOENT){
					if(create_if_missing){
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

		lmdb_minimal* ins = new lmdb_minimal();

		if(MDB_SUCCESS != mdb_env_create(&ins->env)){
			return nullptr;
		}

COUT << ins->env << std::endl;

		if(MDB_SUCCESS != mdb_env_set_maxreaders(ins->env, 256)){
			return nullptr;
		}
COUT << ins->env << std::endl;
		if(MDB_SUCCESS != mdb_env_set_mapsize(ins->env, 2000 * 1024 * 1024 * 1024ul)){
			return nullptr;
		}
COUT << ins->env << std::endl;

		int r = mdb_env_open(ins->env, leveldbpath.c_str(), MDB_FIXEDMAP |MDB_NOSYNC, 0664);
		if(MDB_SUCCESS != r){
COUT << "初始化失败" << strerror(errno) << std::endl;
			return nullptr;
		}
COUT << r  << ",ENOENT=" << ENOENT << ",EACCES=" << EACCES << ",EAGAIN=" << EAGAIN << ",err=" << strerror(errno) << "," << ins->env << std::endl;
		MDB_txn* txn;
		mdb_txn_begin(ins->env, nullptr, 0, &txn);
COUT << ins->env << std::endl;

		mdb_dbi_open(txn, nullptr, 0, &ins->dbi);
		mdb_txn_abort(txn);
COUT << ins->env << std::endl;

		if(MDB_SUCCESS != r){
COUT << "初始化失败" << strerror(errno) << std::endl;
			return nullptr;
		}
COUT << ins->env << std::endl;


		return ins;
	}

	bool lmdb_minimal::get_sync_info(const lyramilk::data::string& masterid,lyramilk::data::string* replid,lyramilk::data::uint64* offset) const
	{
		if(replid == nullptr || offset == nullptr) return false;


		lyramilk::data::string repkey;
		if(masterid.empty()){
			repkey = ".sync.key";
		}else{
			repkey = ".sync.key.";
			repkey += masterid;
		}

		MDB_txn* txn;
		mdb_txn_begin(env, nullptr, MDB_RDONLY, &txn);

		MDB_val key;
		key.mv_size = repkey.size();
		key.mv_data = (char*)repkey.c_str();

		MDB_val data;

		int r = mdb_get(txn,dbi,&key,&data);
		mdb_txn_abort(txn);
		if(r == MDB_SUCCESS){
			std::string stlsync_info((const char*)data.mv_data,data.mv_size);
			*replid = lyramilk::data::str(stlsync_info.substr(sizeof(lyramilk::data::uint64)));
			stlsync_info.copy((char*)offset,sizeof(lyramilk::data::uint64));
			return true;
		}
		return false;
	}

	bool lmdb_minimal::compact()
	{
		TODO();
		/*
		ldb->CompactRange(nullptr,nullptr);
		return true;
		*/
	}

	bool lmdb_minimal::hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		TODO();
		/*
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
		return ldbs.ok();*/
	}

	lyramilk::data::string lmdb_minimal::hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		TODO();
		/*
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
			lyramilk::threading::mutex_sync _(sem);
			leveldb::Status ldbs = ldb->Get(ropt,prefix,&result);
			if(!ldbs.ok()){
				result.clear();
			}
		}
		long long nsec = nd.diff();
		if(nsec > 200000000){
			log(lyramilk::log::warning,"hget") << D("命令 hget %.*s,%.*s 耗时%.3f",key.size(),key.c_str(),field.size(),field.c_str(),double(nsec) / 1000000) << std::endl;
		}
		return lyramilk::data::str(result);*/
	}

	lyramilk::data::stringdict lmdb_minimal::hgetall(const lyramilk::data::string& key) const
	{
		TODO();
		/*
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
			lyramilk::threading::mutex_sync _(sem);
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
		return result;*/
	}

	lyramilk::data::string lmdb_minimal::get_property(const lyramilk::data::string& property)
	{
		TODO();

		/*
		std::string result;
		if(ldb->GetProperty(property,&result)){
			return lyramilk::data::str(result);
		}
		return "";*/
	}

}}
