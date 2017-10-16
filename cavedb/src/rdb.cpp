#include "rdb.h"
extern "C" {
	#include "redis/lzf.h"
	#include "redis/ziplist.h"
	#include "redis/zipmap.h"
	#include "redis/intset.h"
	#include "redis/endianconv.h"
	#include "redis/quicklist.h"
	#include "redis/zmalloc.h"
}

#include <fstream>
#include <libmilk/multilanguage.h>
#include <libmilk/log.h>
#include <libmilk/exception.h>
#include <string.h>
#include <stdlib.h>
#include <endian.h>

#define RDB_VERSION 8
//
#define RDB_6BITLEN 0
#define RDB_14BITLEN 1
#define RDB_32BITLEN 0x80
#define RDB_64BITLEN 0x81
#define RDB_ENCVAL 3
#define RDB_LENERR UINT64_MAX
//
#define RDB_ENC_INT8 0        /* 8 bit signed integer */
#define RDB_ENC_INT16 1       /* 16 bit signed integer */
#define RDB_ENC_INT32 2       /* 32 bit signed integer */
#define RDB_ENC_LZF 3         /* string compressed with FASTLZ */
//
#define RDB_TYPE_STRING 0
#define RDB_TYPE_LIST   1
#define RDB_TYPE_SET    2
#define RDB_TYPE_ZSET   3
#define RDB_TYPE_HASH   4
#define RDB_TYPE_ZSET_2 5 /* ZSET version 2 with doubles stored in binary. */
#define RDB_TYPE_MODULE 6
#define RDB_TYPE_MODULE_2 7
//
#define RDB_TYPE_HASH_ZIPMAP    9
#define RDB_TYPE_LIST_ZIPLIST  10
#define RDB_TYPE_SET_INTSET    11
#define RDB_TYPE_ZSET_ZIPLIST  12
#define RDB_TYPE_HASH_ZIPLIST  13
#define RDB_TYPE_LIST_QUICKLIST 14
//
#define rdbIsObjectType(t) ((t >= 0 && t <= 6) || (t >= 9 && t <= 14))
//
#define RDB_OPCODE_AUX        250
#define RDB_OPCODE_RESIZEDB   251
#define RDB_OPCODE_EXPIRETIME_MS 252
#define RDB_OPCODE_EXPIRETIME 253
#define RDB_OPCODE_SELECTDB   254
#define RDB_OPCODE_EOF        255
//
#define RDB_LOAD_NONE   0
#define RDB_LOAD_ENC    (1<<0)
#define RDB_LOAD_PLAIN  (1<<1)
#define RDB_LOAD_SDS    (1<<2)

#define RDB_SAVE_NONE 0
#define RDB_SAVE_AOF_PREAMBLE (1<<0)


double R_Zero = 0.0;
double R_PosInf = 1.0/0.0;
double R_NegInf = -1.0/0.0;
double R_Nan = 0.0/0.0;


namespace lyramilk{ namespace cave
{
	static lyramilk::log::logss log("lyramilk.cave.rdb");
	rdb::rdb()
	{
	}

	rdb::~rdb()
	{
	}

	lyramilk::data::int64 rdb::load_integer_value(std::istream& is,int enctype, int flags)
	{
		lyramilk::data::int64 lenresult = 0;
		unsigned char enc[4];
		switch(enctype){
		  case RDB_ENC_INT8:{
			is.read((char*)enc,1);
			lyramilk::data::uint8 v = enc[0];
			lenresult = v;
		}break;
		  case RDB_ENC_INT16:{
			is.read((char*)enc,2);
			lyramilk::data::uint16 v = enc[0]|(enc[1]<<8);
			lenresult = v;
		}break;
		  case RDB_ENC_INT32:{
			is.read((char*)enc,4);
			lyramilk::data::uint32 v = enc[0]|(enc[1]<<8)|(enc[2]<<16)|(enc[3]<<24);
			lenresult = v;
		}break;
		  default:
			throw lyramilk::exception(D("redis 整数错误"));
		}
		return lenresult;
	}

	lyramilk::data::string rdb::load_lzf_string(std::istream& is)
	{
		lyramilk::data::uint64 clen = load_length(is,nullptr);
		lyramilk::data::uint64 len = load_length(is,nullptr);

		lyramilk::data::string result;
		result.resize(len);

		std::vector<char> src;
		src.reserve(clen);
		is.read(src.data(),src.capacity());

		if(lzf_decompress(src.data(),src.capacity(),(char*)result.data(),result.size())  == 0){
			throw lyramilk::exception(D("redis 解压缩字符串失败"));
		}
		return result;
	}

	lyramilk::data::uint64 rdb::load_length(std::istream& is,bool* isencode)
	{
		lyramilk::data::uint64 lenresult = 0;
		unsigned char bf = is.get();
		int type = (bf&0xc0)>>6;
		if(type == RDB_ENCVAL){
			if(isencode) *isencode = true;
			lenresult = bf&0x3f;
		}else if(type == RDB_6BITLEN){
			lenresult = bf&0x3f;
		}else if(type == RDB_14BITLEN){
			unsigned char bf2 = is.get();
			lenresult = ((bf&0x3f)<<8)|bf2;
		}else if(bf == RDB_32BITLEN){
			lyramilk::data::uint32 len32;
			is.read((char*)&len32,4);
			lenresult = be32toh(len32);
		}else if(bf == RDB_64BITLEN){
			lyramilk::data::uint32 len64;
			is.read((char*)&len64,8);
			lenresult = be64toh(len64);
		}else{
			throw lyramilk::exception(D("redis 整数错误"));
		}
		return lenresult;
	}

	unsigned char rdb::load_type(std::istream& is)
	{
		return is.get();
	}

	lyramilk::data::int32 rdb::load_time(std::istream& is)
	{
		lyramilk::data::int32 r = 0;
		is.read((char*)&r,4);
		return r;
	}

	lyramilk::data::int64 rdb::load_mtime(std::istream& is)
	{
		lyramilk::data::int64 r = 0;
		is.read((char*)&r,8);
		return r;
	}

	lyramilk::data::var rdb::load_var(std::istream& is)
	{
		bool isencode = false;
		lyramilk::data::int64 len = load_length(is,&isencode);
		if(isencode){
			switch(len) {
			  case RDB_ENC_INT8:
			  case RDB_ENC_INT16:
			  case RDB_ENC_INT32:{
					return load_integer_value(is,len,0);
				}
			  case RDB_ENC_LZF:
				return load_lzf_string(is);
			  default:
				throw lyramilk::exception(D("redis 字符串错误"));
			}
		}
		lyramilk::data::string str;
		str.resize(len);
		is.read((char*)str.data(),len);
		return str;
	}

	lyramilk::data::string rdb::load_string(std::istream& is)
	{
		bool isencode = false;
		lyramilk::data::int64 len = load_length(is,&isencode);
		if(isencode){
			switch(len) {
			  case RDB_ENC_LZF:
				return load_lzf_string(is);
			  default:
				throw lyramilk::exception(D("redis 字符串错误"));
			}
		}
		lyramilk::data::string str;
		str.resize(len);
		is.read((char*)str.data(),len);
		return str;
	}

	double rdb::load_binarydouble(std::istream& is)
	{
		double r;
		is.read((char*)&r,sizeof(r));
		memrev64ifbe(&r);
		return r;
	}

	double rdb::load_double(std::istream& is)
	{
		char buf[256];
		unsigned char len = is.get();
		switch(len){
		  case 255:return R_NegInf;
		  case 254:return R_PosInf;
		  case 253:return R_Nan;
		  default:{
				double val;
				is.read(buf,len);
				buf[len] = 0;
				sscanf(buf,"%lg",&val);
				return val;
			}
		}
	}

	void rdb::parse_object(std::istream& is,lyramilk::data::uint64 dbid,lyramilk::data::uint64 expiretime,int type,const lyramilk::data::string& key)
	{
		switch(type){
		  case RDB_TYPE_STRING:{
				lyramilk::data::string str = load_var(is);
				notify_set(key,str);
				return;
			}break;
		  case RDB_TYPE_LIST:{
				lyramilk::data::uint64 len = load_length(is,nullptr);
				for(lyramilk::data::uint64 i=0;i<len;++i){
					lyramilk::data::string value = load_var(is);
					notify_rpush(key,value);
				}
				return;
			}break;
		  case RDB_TYPE_SET:{
				lyramilk::data::uint64 len = load_length(is,nullptr);
				for(lyramilk::data::uint64 i=0;i<len;++i){
					lyramilk::data::string value = load_var(is);
					notify_sadd(key,value);
				}
				return;
			}break;
		  case RDB_TYPE_ZSET:
		  case RDB_TYPE_ZSET_2:{
				lyramilk::data::uint64 len = load_length(is,nullptr);
				for(lyramilk::data::uint64 i=0;i<len;++i){
					lyramilk::data::string value = load_var(is);
					double score;
					if(type == RDB_TYPE_ZSET_2){
						score = load_binarydouble(is);
					}else{
						score = load_double(is);
					}
					notify_zadd(key,value,score);
				}
				return;
			}break;
		  case RDB_TYPE_HASH:{
				lyramilk::data::uint64 len = load_length(is,nullptr);
				for(lyramilk::data::uint64 i=0;i<len;++i){
					lyramilk::data::string field = load_var(is);
					lyramilk::data::var value = load_var(is);
					notify_hset(key,field,value);
				}
				return;
			}break;
		  case RDB_TYPE_LIST_QUICKLIST:{
				lyramilk::data::uint64 len = load_length(is,nullptr);
				quicklist *l = quicklistCreate();
				quicklistSetOptions(l,-2,0);
				while(len--){
					lyramilk::data::string str = load_var(is);
					unsigned char *zl = (unsigned char*)zmalloc(str.size());
					str.copy((char*)zl,str.size());
					if(zl == nullptr){
						log(lyramilk::log::error,"init") << D("redis 文件格式错误") << std::endl;
					}
					quicklistAppendZiplist(l,zl);
				}

				quicklistIter* iter = quicklistGetIterator(l,AL_START_HEAD);
				quicklistEntry entry;
				while (quicklistNext(iter, &entry)) {
					lyramilk::data::var v;
					if(entry.value){
						v = lyramilk::data::string((const char*)entry.value,entry.sz);
					}else{
						v = entry.longval;
					}
					notify_rpush(key,v.str());
				}
				quicklistReleaseIterator(iter);
				quicklistRelease(l);
				return;
			}break;
		  case RDB_TYPE_HASH_ZIPMAP:{
				lyramilk::data::string str = load_string(is);
				unsigned char *fstr, *vstr;
				unsigned int flen, vlen;
				unsigned char *zl = (unsigned char*)str.c_str();
				unsigned char *zi = zipmapRewind(zl);
				while ((zi = zipmapNext(zi, &fstr, &flen, &vstr, &vlen)) != NULL) {
					lyramilk::data::string field((const char*)fstr,flen);
					lyramilk::data::string value((const char*)vstr,vlen);
					notify_hset(key,field,value);
				}
				return;
			}break;
		  case RDB_TYPE_LIST_ZIPLIST:{
				lyramilk::data::string str = load_string(is);
				unsigned char *zl = (unsigned char*)str.c_str();

				unsigned int sz = ziplistLen(zl);
				for(unsigned int i=0;i <sz; i+=2){
					lyramilk::data::var value;
					{
						unsigned char *sptr = ziplistIndex(zl,i);
						unsigned char* sval;
						unsigned int slen = 0;
						long long lval;

						if(!ziplistGet(sptr,&sval,&slen,&lval)){
							log(lyramilk::log::error,"init") << D("redis 文件格式错误") << std::endl;
						}
						if(sval){
							value.assign(lyramilk::data::string((const char*)sval,slen));
						}else{
							value.assign(lval);
						}
						sptr = ziplistNext(zl,sptr);
						if(!sptr){
							log(lyramilk::log::error,"init") << D("redis 文件格式错误") << std::endl;
						}
					}
					notify_rpush(key,value);
				}
				return;
			}break;
		  case RDB_TYPE_SET_INTSET:{
				lyramilk::data::string str = load_string(is);
				intset *zl = (intset*)str.c_str();
				uint32_t sz = intsetLen(zl);
				for(uint32_t i=0;i<sz;++i){
					int64_t value;
					intsetGet(zl,i,&value);
					char buff[256];
					int i = snprintf(buff,sizeof(buff),"%lld",(long long)value);
					notify_sadd(key,lyramilk::data::string(buff,i));
				}
				return;
			}break;
		  case RDB_TYPE_ZSET_ZIPLIST:{
				lyramilk::data::string str = load_string(is);
				unsigned char *zl = (unsigned char*)str.c_str();

				unsigned int sz = ziplistLen(zl);
				for(unsigned int i=0;i <sz; i+=2){
					lyramilk::data::var value;
					{
						unsigned char *sptr = ziplistIndex(zl,i);
						unsigned char* sval;
						unsigned int slen = 0;
						long long lval;

						if(!ziplistGet(sptr,&sval,&slen,&lval)){
							log(lyramilk::log::error,"init") << D("redis 文件格式错误") << std::endl;
						}
						if(sval){
							value.assign(lyramilk::data::string((const char*)sval,slen));
						}else{
							value.assign(lval);
						}
						sptr = ziplistNext(zl,sptr);
						if(!sptr){
							log(lyramilk::log::error,"init") << D("redis 文件格式错误") << std::endl;
						}
					}
					double score;
					{
						unsigned char *sptr = ziplistIndex(zl,i+1);
						unsigned char* sval;
						unsigned int slen = 0;
						long long lval;
							
						if(!ziplistGet(sptr,&sval,&slen,&lval)){
							log(lyramilk::log::error,"init") << D("redis 文件格式错误") << std::endl;
						}

						if (sval) {
							memcpy(sval,sval,slen);
							sval[slen] = '\0';
							score = strtod((const char*)sval,NULL);
						} else {
							score = lval;
						}
					}
					notify_zadd(key,value,score);
				}
				return;
			}break;
		  case RDB_TYPE_HASH_ZIPLIST:{
				lyramilk::data::string str = load_var(is);

				unsigned char *zl = (unsigned char*)str.c_str();
				unsigned char *sptr = ziplistIndex(zl,ZIPLIST_HEAD);
				for(;sptr;sptr = ziplistNext(zl,sptr)){
					lyramilk::data::var field;
					{
						unsigned char* sval;
						unsigned int slen = 0;
						long long lval;
							
						if(!ziplistGet(sptr,&sval,&slen,&lval)){
							log(lyramilk::log::error,"init") << D("redis 文件格式错误") << std::endl;
						}
						if(sval){
							field.assign(lyramilk::data::string((const char*)sval,slen));
						}else{
							field.assign(lval);
						}
						sptr = ziplistNext(zl,sptr);
						if(!sptr){
							log(lyramilk::log::error,"init") << D("redis 文件格式错误") << std::endl;
						}
					}
					lyramilk::data::var value;
					{
						unsigned char* sval;
						unsigned int slen = 0;
						long long lval;
							
						if(!ziplistGet(sptr,&sval,&slen,&lval)){
							log(lyramilk::log::error,"init") << D("redis 文件格式错误") << std::endl;
						}
						if(sval){
							value.assign(lyramilk::data::string((const char*)sval,slen));
						}else{
							value.assign(lval);
						}
					}
					notify_hset(key,field,value);
				}
				return;
			}break;
		  case RDB_TYPE_MODULE_2:
		  case RDB_TYPE_MODULE:{
				lyramilk::data::uint64 moduleid = load_length(is,nullptr);
				log(lyramilk::log::error,"init") << D("redis 加载redis-module错误：moduleid=%llu",moduleid) << std::endl;
			}break;
		  default:
			log(lyramilk::log::error,"init") << D("redis 文件格式错误：key=%s,value=%d",key.c_str(),type) << std::endl;
		}
	}

	bool rdb::init(const lyramilk::data::string& filename)
	{
		std::ifstream ifs(filename.c_str(),std::ifstream::binary|std::ifstream::in);
		return init(ifs);

	}

	bool rdb::init(std::istream& is)
	{
		char buf[1024];
		is.read(buf,9);

		if (memcmp(buf,"REDIS",5) != 0) {
			log(lyramilk::log::error,"init") << D("redis 文件格式错误") << std::endl;
			return false;
		}

		unsigned int version = atoi(buf+5);
		log(lyramilk::log::debug,"init") << D("redis 版本号%d",version) << std::endl;
		lyramilk::data::uint64 dbid = 0;

		while(true){
			lyramilk::data::int64 expiretime = 0;
			int type = load_type(is);

			if (type == RDB_OPCODE_EXPIRETIME) {
				expiretime = load_time(is);
				expiretime *= 1000;
				type = load_type(is);
			} else if (type == RDB_OPCODE_EXPIRETIME_MS) {
				expiretime = load_mtime(is);
				type = load_type(is);
			} else if (type == RDB_OPCODE_EOF) {
				lyramilk::data::uint64 cksum;
				is.read((char*)&cksum,sizeof(cksum));
				memrev64ifbe(&cksum);
				log(lyramilk::log::debug,__FUNCTION__) << D("rdb处理完成%x",cksum) << std::endl;
				return true;
			} else if (type == RDB_OPCODE_SELECTDB) {
				dbid = load_length(is,nullptr);
				notify_select(dbid);
				continue;
			} else if (type == RDB_OPCODE_RESIZEDB) {
				lyramilk::data::uint64 db_size,expires_size;
				db_size = load_length(is,nullptr);
				expires_size = load_length(is,nullptr);
				continue;
			} else if (type == RDB_OPCODE_AUX) {
				lyramilk::data::var key = load_var(is);
				lyramilk::data::var value = load_var(is);
				notify_aux(key,value);
				continue;
			}
			lyramilk::data::var key = load_var(is);
			parse_object(is,dbid,expiretime,type,key);
			if(expiretime > 0){
				notify_pexpireat(key,expiretime);
			}
		}
		log(lyramilk::log::debug,"init") << D("redis 文件解析成功") << std::endl;
		return true;
	}

	bool rdb::restore(std::istream& is,lyramilk::data::uint64 dbid,lyramilk::data::uint64 expiretime,const lyramilk::data::string& key)
	{
		int type = load_type(is);
		parse_object(is,dbid,expiretime,type,key);
		return true;
	}

	void rdb::notify_select(lyramilk::data::uint64 dbid)
	{
		//log(lyramilk::log::debug,__FUNCTION__) << "select " << dbid << std::endl;
	}

	void rdb::notify_aux(const lyramilk::data::string& key,const lyramilk::data::var& value)
	{
		//log(lyramilk::log::debug,__FUNCTION__) << key << "=" << value << std::endl;
	}

	void rdb::notify_hset(const lyramilk::data::string& key,const lyramilk::data::string& field,const lyramilk::data::var& value)
	{
		//log(lyramilk::log::debug,__FUNCTION__) << "hset " << key << "," << field << "," << value << std::endl;
	}

	void rdb::notify_zadd(const lyramilk::data::string& key,const lyramilk::data::var& value,double score)
	{
		//log(lyramilk::log::debug,__FUNCTION__) << "zadd " << key << "," << score << "," << value << std::endl;
	}

	void rdb::notify_set(const lyramilk::data::string& key,const lyramilk::data::string& value)
	{
		//log(lyramilk::log::debug,__FUNCTION__) << "set " << key << "," << value << std::endl;
	}

	void rdb::notify_rpush(const lyramilk::data::string& key,const lyramilk::data::string& item)
	{
		//log(lyramilk::log::debug,__FUNCTION__) << "rpush " << key << "," << item << std::endl;
	}

	void rdb::notify_sadd(const lyramilk::data::string& key,const lyramilk::data::string& value)
	{
		//log(lyramilk::log::debug,__FUNCTION__) << "sadd " << key << "," << value << std::endl;
	}

	void rdb::notify_pexpireat(const lyramilk::data::string& key,lyramilk::data::uint64 expiretime)
	{
		//log(lyramilk::log::debug,__FUNCTION__) << "pexpireat " << key << "," << expiretime << std::endl;
	}


}}
