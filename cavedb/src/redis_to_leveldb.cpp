#include "redis_to_leveldb.h"
#include "leveldb_util.h"
#include <libmilk/exception.h>
#include <libmilk/multilanguage.h>
#include <stdlib.h>
#include <sys/time.h>

namespace lyramilk{ namespace cave
{
	redis_leveldb_key::redis_leveldb_key()
	{
		str.reserve(1024);
	}

	redis_leveldb_key::redis_leveldb_key(const std::string& prefix)
	{
		str.reserve(1024);
		str.append(prefix.c_str(),prefix.size());
	}

	redis_leveldb_key::~redis_leveldb_key()
	{}

	void redis_leveldb_key::append_string(const char* pkey,std::size_t sz)
	{
		str.push_back(KEY_STR);
		str.append(pkey,sz);
	}

	void redis_leveldb_key::append_int(lyramilk::data::uint64 d)
	{
		str.push_back(KEY_INT);
		lyramilk::data::uint64 bd = htobe64(d);
		str.append((char*)&bd,sizeof(bd));
	}

	void redis_leveldb_key::append_rint(lyramilk::data::uint64 d)
	{
		str.push_back(KEY_RINT);
		lyramilk::data::uint64 bd = htobe64(d);
		str.append((char*)&bd,sizeof(bd));
	}

	void redis_leveldb_key::append_double(double d)
	{
		str.push_back(KEY_DOUBLE);
		str.append((char*)&d,sizeof(d));
	}

	void redis_leveldb_key::append_type(unsigned char kt)
	{
		str.push_back(KEY_STR);
		str.push_back(kt);
	}

	void redis_leveldb_key::append_noop()
	{
		str.push_back(KEY_NOOP);
	}

	void redis_leveldb_key::append_max()
	{
		str.push_back(KEY_EOF);
	}

	redis_leveldb_key::operator std::string()
	{
		return str;
	}

	redis_leveldb_key::operator leveldb::Slice()
	{
		return str;
	}

	redis_leveldb_comparator::redis_leveldb_comparator()
	{
	}

	redis_leveldb_comparator::~redis_leveldb_comparator()
	{
	}

	int redis_leveldb_comparator::Compare(const leveldb::Slice& a, const leveldb::Slice& b) const
	{
		std::size_t sz = std::min(a.size(),b.size());
		for(std::size_t i =0;i<sz;++i){
			unsigned char c = a[i];
			unsigned char d = b[i];
			if(c == redis_leveldb_key::KEY_INT && d == redis_leveldb_key::KEY_INT){
				if(i + sizeof(lyramilk::data::uint64) >= sz){
					goto label_prefix_eq;
				}
				for(unsigned int n=0;n<sizeof(lyramilk::data::uint64);++n){
					unsigned char c = a[i + n + 1];
					unsigned char d = b[i + n + 1];
					if(c < d) return -1;
					if(c > d) return 1;
				}
				i += sizeof(lyramilk::data::uint64);
			}else if(c == redis_leveldb_key::KEY_RINT && d == redis_leveldb_key::KEY_RINT){
				if(i + sizeof(lyramilk::data::uint64) >= sz){
					goto label_prefix_eq;
				}
				for(unsigned int n=0;n<sizeof(lyramilk::data::uint64);++n){
					unsigned char c = a[i + n + 1];
					unsigned char d = b[i + n + 1];
					if(c < d) return 1;
					if(c > d) return -1;
				}
				i += sizeof(lyramilk::data::uint64);
			}else if(c == redis_leveldb_key::KEY_DOUBLE && d == redis_leveldb_key::KEY_DOUBLE){
				if(i + sizeof(double) >= sz){
					goto label_prefix_eq;
				}
				double c = *(double*)(a.data() + i + 1);
				double d = *(double*)(b.data() + i + 1);
				if(c < d) return -1;
				if(c > d) return 1;
				i += sizeof(double);
			}else{
				if(c < d) return -1;
				if(c > d) return 1;
			}
		}

label_prefix_eq:
		if(a.size() == b.size()) return 0;
		return a.size() < b.size() ? -1 : 1;
	}

	const char* redis_leveldb_comparator::Name() const
	{
		return "cavedb.KeyComparator";
		//return "leveldb.BytewiseComparator";
	}

	void redis_leveldb_comparator::FindShortestSeparator(std::string* start,const leveldb::Slice& limit) const
	{
		/*
		std::string& a = *start;
		const leveldb::Slice& b = limit;

		std::size_t sz = std::min(a.size(),b.size());
		for(std::size_t i =0;i<sz;++i){
			unsigned char c = (unsigned char)a[i];
			unsigned char d = (unsigned char)b[i];
			if(c == redis_leveldb_key::KEY_DOUBLE && d == redis_leveldb_key::KEY_DOUBLE){
				if(i + sizeof(double) >= sz){
					return;
				}
				double c = *(double*)(a.data() + i + 1);
				double d = *(double*)(b.data() + i + 1);
				if(c != d){
					double q = (c + d / 2);
					memcpy((char*)a.data() + i + 1,&q,sizeof(double));
					a.resize(i + sizeof(double));
					assert(Compare(*start, limit) < 0);
					return;
				}
				i += sizeof(double);
			}else if(c != d){
				if(c + 1 < d){
					++a[i];
					a.resize(i+1);
					assert(Compare(*start, limit) < 0);
					return;
				}
				return;
			}
		}*/
	}

	void redis_leveldb_comparator::FindShortSuccessor(std::string* key) const
	{
		/*
		std::size_t n = key->size();
		for (std::size_t i = 0; i < n; i++) {
			const unsigned char byte = (*key)[i];
			if (byte != static_cast<uint8_t>(0xff)) {
				(*key)[i] = byte + 1;
				key->resize(i+1);
				return;
			}
		}*/
	}

	redis_leveldb_handler::redis_leveldb_handler()
	{
		userdata = nullptr;
		idbid = -2;
		select(0);
	}

	redis_leveldb_handler::~redis_leveldb_handler()
	{
	}

	lyramilk::data::int64 redis_leveldb_handler::mstime()
	{
		struct timeval tv;
		gettimeofday(&tv,NULL);
		lyramilk::data::int64 t = tv.tv_sec;
		t *= 1000;
		return t + tv.tv_usec/1000;
	}

	std::string redis_leveldb_handler::double2bytes(double v)
	{
		return std::string((char*)&v,sizeof(v));
	}

	double redis_leveldb_handler::bytes2double(const lyramilk::data::string& str)
	{
		return *(double*)str.c_str();
	}

	double redis_leveldb_handler::bytes2double(const leveldb::Slice& str)
	{
		return *(double*)str.data();
	}

	std::string redis_leveldb_handler::integer2bytes(lyramilk::data::uint64 v)
	{
		return std::string((char*)&v,sizeof(v));
	}

	lyramilk::data::uint64 redis_leveldb_handler::bytes2integer(const lyramilk::data::string& str)
	{
		return *(lyramilk::data::uint64*)str.data();
	}

	lyramilk::data::uint64 redis_leveldb_handler::bytes2integer(const leveldb::Slice& str)
	{
		return *(lyramilk::data::uint64*)str.data();
	}

	std::string redis_leveldb_handler::dbprefix(lyramilk::data::uint64 dbid)
	{
		if(dbid == (lyramilk::data::uint64)-1 || dbid == idbid){
			return this->dbid;
		}

		redis_leveldb_key k;
		k.append_string("d",1);
		k.append_int(dbid);
		return k;
	}

	std::string redis_leveldb_handler::encode_ttl_key(const lyramilk::data::string& key,lyramilk::data::uint64 dbid)
	{
		redis_leveldb_key k(dbprefix(dbid));
		k.append_type(redis_leveldb_key::KT_BASE);
		k.append_string(key.c_str(),key.size());
		k.append_string("ttl",3);
		return k;
	}

	std::string redis_leveldb_handler::encode_size_key(const lyramilk::data::string& key,lyramilk::data::uint64 dbid)
	{
		redis_leveldb_key k(dbprefix(dbid));
		k.append_type(redis_leveldb_key::KT_BASE);
		k.append_string(key.c_str(),key.size());
		k.append_string("size",4);
		return k;
	}

	std::string redis_leveldb_handler::encode_string_key(const lyramilk::data::string& key,lyramilk::data::uint64 dbid)
	{
		redis_leveldb_key k(dbprefix(dbid));
		k.append_type(redis_leveldb_key::KT_STRING);
		k.append_string(key.c_str(),key.size());
		k.append_string("v",1);
		return k;
	}

	std::string redis_leveldb_handler::encode_zset_score_key(const lyramilk::data::string& key,double score,const lyramilk::data::string& value,lyramilk::data::uint64 dbid)
	{
		redis_leveldb_key k(dbprefix(dbid));
		k.append_type(redis_leveldb_key::KT_ZSET);
		k.append_string(key.c_str(),key.size());
		k.append_string("s",1);
		k.append_double(score);
		k.append_string(value.c_str(),value.size());
		return k;
	}

	std::string redis_leveldb_handler::encode_zset_data_key(const lyramilk::data::string& key,const lyramilk::data::string& value,lyramilk::data::uint64 dbid)
	{
		redis_leveldb_key k(dbprefix(dbid));
		k.append_type(redis_leveldb_key::KT_ZSET);
		k.append_string(key.c_str(),key.size());
		k.append_string("v",1);
		k.append_string(value.c_str(),value.size());
		return k;
	}

	std::string redis_leveldb_handler::encode_set_data_key(const lyramilk::data::string& key,const lyramilk::data::string& value,lyramilk::data::uint64 dbid)
	{
		redis_leveldb_key k(dbprefix(dbid));
		k.append_type(redis_leveldb_key::KT_SET);
		k.append_string(key.c_str(),key.size());
		k.append_string("v",1);
		k.append_string(value.c_str(),value.size());
		return k;
	}

	std::string redis_leveldb_handler::encode_hashmap_data_key(const lyramilk::data::string& key,const lyramilk::data::string& field,lyramilk::data::uint64 dbid)
	{
		redis_leveldb_key k(dbprefix(dbid));
		k.append_type(redis_leveldb_key::KT_HASH);
		k.append_string(key.c_str(),key.size());
		k.append_string("f",1);
		k.append_string(field.c_str(),field.size());
		return k;
	}

	std::string redis_leveldb_handler::encode_list_data_key(const lyramilk::data::string& key,lyramilk::data::uint64 seq,lyramilk::data::uint64 dbid)
	{
		redis_leveldb_key k(dbprefix(dbid));
		k.append_type(redis_leveldb_key::KT_LIST);
		k.append_string(key.c_str(),key.size());
		k.append_string("q",1);
		k.append_int(seq);
		return k;
	}

	void redis_leveldb_handler::init(leveldb::DB* pldb)
	{
		this->ldb = pldb;
	}

	void redis_leveldb_handler::select(lyramilk::data::uint64 dbid)
	{
		this->dbid = dbprefix(dbid);
		this->idbid = dbid;
	}

	lyramilk::data::uint64 redis_leveldb_handler::get_dbid()
	{
		return idbid;
	}

	lyramilk::data::uint64 redis_leveldb_handler::get_size(const lyramilk::data::string& key,lyramilk::data::uint64 dbid)
	{
		std::string size_key = encode_size_key(key,dbid);
		std::string ssize;
		ldb->Get(ropt,size_key,&ssize);
		return bytes2integer(ssize);
	}

	bool redis_leveldb_handler::incr_size(leveldb::WriteBatch &batch,const lyramilk::data::string& key)
	{
		std::string size_key = encode_size_key(key,idbid);
		std::string ssize;
		leveldb::Status ldbs = ldb->Get(ropt,size_key,&ssize);
		if(!ldbs.ok()){
			if(ldbs.IsNotFound()){
				batch.Put(size_key,integer2bytes(1));
				return true;
			}
			return false;
		}
		lyramilk::data::uint64 sz = bytes2integer(ssize);
		ssize = integer2bytes(sz + 1);
		batch.Put(size_key,ssize);
		return true;
	}

	bool redis_leveldb_handler::decr_size(leveldb::WriteBatch &batch,const lyramilk::data::string& key)
	{
		std::string size_key = encode_size_key(key,idbid);
		std::string ssize;
		ldb->Get(ropt,size_key,&ssize);
		lyramilk::data::uint64 sz = bytes2integer(ssize);
		if(sz < 1) return false;
		ssize = integer2bytes(sz - 1);
		batch.Put(size_key,ssize);
		return true;
	}

	void redis_leveldb_handler::set_size(leveldb::WriteBatch &batch,const lyramilk::data::string& key,lyramilk::data::uint64 size)
	{
		std::string size_key = encode_size_key(key,idbid);
		std::string ssize = integer2bytes(size);
		batch.Put(size_key,ssize);
	}

	lyramilk::data::int64 redis_leveldb_handler::get_pttl(const lyramilk::data::string& key,lyramilk::data::uint64 dbid)
	{
		std::string ttl_key = encode_size_key(key,dbid);
		std::string ttl_str;
		ldb->Get(ropt,ttl_key,&ttl_str);
		return bytes2integer(ttl_str);
	}

	void redis_leveldb_handler::set_pttl(leveldb::WriteBatch &batch,const lyramilk::data::string& key,lyramilk::data::int64 expiretime)
	{
		std::string ttl_key = encode_ttl_key(key,idbid);
		std::string ttl_str = integer2bytes(expiretime);
		batch.Put(ttl_key,ttl_str);
	}

	void cavedb_noop(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
	
	}

	cavedb_redis_commands::cavedb_redis_commands()
	{
	}

	cavedb_redis_commands::~cavedb_redis_commands()
	{
	}

	cavedb_redis_commands* cavedb_redis_commands::instance()
	{
		static cavedb_redis_commands _mm;
		return &_mm;
	}
}}
