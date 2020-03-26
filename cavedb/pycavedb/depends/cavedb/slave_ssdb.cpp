#include "slave_ssdb.h"
#include "const.h"
#include <libmilk/dict.h>
#include <libmilk/log.h>
#include <libmilk/exception.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <endian.h>


const uint64_t QFRONT_SEQ = 2;
const uint64_t QBACK_SEQ  = 3;
const uint64_t QITEM_MIN_SEQ = 10000;
const uint64_t QITEM_MAX_SEQ = 9223372036854775807ULL;
const uint64_t QITEM_SEQ_INIT = QITEM_MAX_SEQ/2;


namespace lyramilk{ namespace cave
{
	static lyramilk::log::logss log("lyramilk.cave.slave_ssdb");

	bool inline parse_ssdb(std::istream& is,lyramilk::data::strings& ret)
	{
		lyramilk::data::string str;
		str.reserve(512);
		char c = 0;
		while(is.good()){
			while(is.good()){
				c = is.get();
label_bodys:
				if(c >= '0' && c <= '9'){
					str.push_back(c);
				}
				if((c == '\r' && '\n' == is.get()) || c == '\n'){
					break;
				}
			}
			char* p;
			lyramilk::data::uint64 sz = strtoull(str.c_str(),&p,10);
			if(sz>0 || (str.size() > 0 && str[0] == '0')){
				str.clear();
				while(sz>0){
					str.push_back(is.get());
					--sz;
				}
				ret.push_back(str);
				c = is.get();
				if((c == '\r' && '\n' == is.get()) || c == '\n'){
					c = is.get();
					if(c >= '0' && c <= '9'){
						str.clear();
						goto label_bodys;
					}
					if((c == '\r' && '\n' == is.get()) || c == '\n'){
						return true;
					}
					log(lyramilk::log::error,"parse") << D("ssdb 错误：响应格式错误%d",1) << std::endl;
					return false;
				}
				log(lyramilk::log::error,"parse") << D("ssdb 错误：响应格式错误%d",2) << std::endl;
				return false;
			}
			log(lyramilk::log::error,"parse") << D("ssdb 错误：响应格式错误str=") << str << std::endl;
			return false;
		}
		log(lyramilk::log::error,"parse") << D("ssdb 错误：响应格式错误%d",4) << std::endl;
		//throw lyramilk::exception(D("ssdb 错误：响应格式错误%d",(unsigned int)c));
		return false;
	}

	slave_ssdb::slave_ssdb()
	{
		status = st_idle;
		psync_replid = "";
		psync_offset = 0;
		loadsum = 0;
	}

	slave_ssdb::~slave_ssdb()
	{
		status = st_stop;
	}

	lyramilk::data::uint64 slave_ssdb::tell_offset()
	{
		return psync_offset;
	}

	void slave_ssdb::slaveof(const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& pwd,lyramilk::data::string psync_replid,lyramilk::data::uint64 psync_offset,slave* peventhandler)
	{
		this->host = host;
		this->port = port;
		this->pwd = pwd;
		this->peventhandler = peventhandler;
		this->psync_replid = psync_replid;
		this->psync_offset = psync_offset;
		active(1);
	}

	void slave_ssdb::init(const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& pwd,lyramilk::data::string psync_replid,lyramilk::data::uint64 psync_offset,slave* peventhandler)
	{
		this->host = host;
		this->port = port;
		this->pwd = pwd;
		this->peventhandler = peventhandler;
		this->psync_replid = psync_replid;
		this->psync_offset = psync_offset;
	}

	lyramilk::data::string slave_ssdb::hexmem(const void *p, int size)
	{
		int capacity = (size < 20)? 32 : (size * 1.2);
		lyramilk::data::string ret;
		ret.reserve(capacity);

		const char* s = (const char*)p;
		static const char *hex = "0123456789abcdef";
		for(int i=0; i<size; i++){
			char c = s[i];
			switch(c){
				case '\r':
					ret.append("\\r");
					break;
				case '\n':
					ret.append("\\n");
					break;
				case '\t':
					ret.append("\\t");
					break;
				case '\\':
					ret.append("\\\\");
					break;
				case ' ':
					ret.push_back(c);
					break;
				default:
					if(c >= '!' && c <= '~'){
						ret.push_back(c);
					}else{
						ret.append("\\x");
						unsigned char d = c;
						ret.push_back(hex[d >> 4]);
						ret.push_back(hex[d & 0x0f]);
					}
					break;
			}
		}
		return ret;
	}

	bool slave_ssdb::reconnect()
	{
		c.close();
		if(c.open(host,port)){
			is.init(&c);
			if(!pwd.empty()){
				lyramilk::data::array ar;
				ar.push_back("auth");
				ar.push_back(pwd);
				if(push(ar)){
					lyramilk::data::strings ret;
					pop(&ret);
					if(ret.size() == 1 && ret[0] == "OK") return true;
				}
				return false;
			}
			return true;
		}
		return false;
	}

	bool slave_ssdb::exec(const lyramilk::data::array& cmd,lyramilk::data::strings* ret)
	{
		if(push(cmd)){
			pop(ret);
			return true;
		}
		return false;
	}

	bool slave_ssdb::push(const lyramilk::data::array& cmd)
	{
		lyramilk::data::stringstream ss;
		{
			lyramilk::data::array::const_iterator it = cmd.begin();
			for(;it!=cmd.end();++it){
				lyramilk::data::string str = it->str();
				ss << str.size() << "\n";
				ss << str << "\n";
			}
			ss << "\n";
			ss.flush();
		}
		lyramilk::data::string str = ss.str();
		return c.write(str.c_str(),str.size()) == (int)str.size();
	}

	bool slave_ssdb::pop(lyramilk::data::strings* ret)
	{
		if(!parse_ssdb(is,*ret)){
			ret->clear();
			c.close();
			return false;
		}

		return true;
	}

	int slave_ssdb::svc()
	{
		while(status != st_stop){
			try{
				if(!(is.good() && c.isalive())){
					reconnect();
					lyramilk::data::array ar;
					ar.push_back("sync140");
					ar.push_back(psync_offset);
					ar.push_back(psync_replid);
					ar.push_back("sync");
					if(!push(ar)){
						c.close();
						continue;
					}
				}
				if(c.check_read(5000)){
					lyramilk::data::strings reply;
					//parse_ssdb2(is,reply);
					pop(&reply);
					if(reply.size() > 0){
						lyramilk::data::uint64 seq = *(lyramilk::data::uint64*)reply[0].c_str();
						char type = reply[0].c_str()[sizeof(lyramilk::data::uint64)];
						char cmd = reply[0].c_str()[sizeof(lyramilk::data::uint64) + 1];
						const static int cmdoffset = sizeof(lyramilk::data::uint64) + 1 + 1;
						switch(type){
						  case BinlogType::NOOP:
								proc_noop(seq);
							break;
						  case BinlogType::CTRL:
							if(strcmp("OUT_OF_SYNC",reply[0].c_str() + cmdoffset) == 0){
								log(lyramilk::log::error,"psync") << D("同步错误:%s","OUT_OF_SYNC") << std::endl;
								lyramilk::data::array ar;
								ar.push_back("sync_overflow");
								if(!peventhandler->notify_command(psync_replid,0,ar)){
									status = st_stop;
								}
								psync_offset = 0;
								psync_replid = "";
								c.close();
								continue;
							}
							break;
						  case BinlogType::COPY:
							if(!proc_copy(seq,cmd,reply[0].c_str() + cmdoffset,reply[0].size() - cmdoffset,reply)){
								status = st_stop;
							}
							break;
						  case BinlogType::SYNC:
						  case BinlogType::MIRROR:
							psync_offset = seq;
							if(!proc_sync(seq,cmd,reply[0].c_str() + cmdoffset,reply[0].size() - cmdoffset,reply)){
								status = st_stop;
							}
							break;
						}
					}
				}else{
					if(!peventhandler->notify_idle(psync_replid,psync_offset)){
						status = st_stop;
					}
				}
			}catch(lyramilk::exception& e){
				log(lyramilk::log::error,"psync.catch") << e.what() << std::endl;
			}
		}
		log(lyramilk::log::error,"psync") << D("同步线程退出") << std::endl;
		return 0;
	}

	bool slave_ssdb::proc_noop(lyramilk::data::uint64 seq)
	{
		return peventhandler->notify_idle(psync_replid,psync_offset);
	}

	bool slave_ssdb::proc_copy(lyramilk::data::uint64 seq,char cmd,const char* p,std::size_t l,const lyramilk::data::strings& args)
	{
		switch(cmd){
		  case BinlogCommand::BEGIN:
			peventhandler->is_in_full_sync = true;
			log(lyramilk::log::debug,"proc_copy") << D("拷贝开始") << std::endl;
			{
				lyramilk::data::array ar;
				ar.push_back("sync_start");
				return peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::END:
			psync_replid = "";
			if(peventhandler->notify_psync(psync_replid,seq)){
				log(lyramilk::log::debug,"proc_copy") << D("拷贝结束") << std::endl;
				psync_offset = seq;
				peventhandler->is_in_full_sync = false;
				lyramilk::data::array ar;
				ar.push_back("sync_continue");
				return peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			log(lyramilk::log::error,"proc_copy") << D("拷贝出错") << std::endl;
			return false;
			break;
		  default:
			psync_replid.assign(p,l);
			return proc_sync(seq,cmd,p,l,args);
		}
		return false;
	}

	bool slave_ssdb::proc_sync(lyramilk::data::uint64 seq,char cmd,const char* p,std::size_t l,const lyramilk::data::strings& args)
	{
		switch(cmd){
		  case BinlogCommand::KSET:
			{
				if(args.size() != 2){
					log(lyramilk::log::error,"psync") << D("同步错误:set 参数过少") << std::endl;
					break;
				}
				lyramilk::data::string tab(p+1,l-1);

				lyramilk::data::array ar;
				ar.reserve(3);
				ar.push_back("set");
				ar.push_back(tab);
				ar.push_back(args[1]);
				return peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::KDEL:
			{
				lyramilk::data::string tab(p+1,l-1);

				lyramilk::data::array ar;
				ar.reserve(3);
				ar.push_back("ssdb_del");
				ar.push_back(tab);
				return peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::HSET:
			{
				if(args.size() != 2){
					log(lyramilk::log::error,"psync") << D("同步错误:hset 参数过少") << std::endl;
					break;
				}
				unsigned int len = (unsigned int)p[1]&0xff;
				lyramilk::data::string tab(1+p+1,len);
				lyramilk::data::string key(1+p+1+len+1,l-2-len-1);

				lyramilk::data::array ar;
				ar.reserve(4);
				ar.push_back("hset");
				ar.push_back(tab);
				ar.push_back(key);
				ar.push_back(args[1]);
				return peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::HDEL:
			{
				unsigned int len = (unsigned int)p[1]&0xff;
				lyramilk::data::string tab(1+p+1,len);
				lyramilk::data::string key(1+p+1+len+1,l-2-len-1);

				lyramilk::data::array ar;
				ar.reserve(3);
				ar.push_back("hdel");
				ar.push_back(tab);
				ar.push_back(key);
				return peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::ZSET:
			{
				if(args.size() != 2){
					log(lyramilk::log::error,"psync") << D("同步错误:zadd 参数过少") << std::endl;
					break;
				}

				unsigned int len = (unsigned int)p[1]&0xff;
				lyramilk::data::string tab(1+p+1,len);
				unsigned int len2 = (unsigned int)p[1+1+len]&0xff;
				lyramilk::data::string key(1+p+1+len+1,len2);

				const lyramilk::data::string& sscore = args[1];

				char* p;
				long long score = strtoll(sscore.c_str(),&p,10);

				lyramilk::data::array ar;
				ar.reserve(4);
				ar.push_back("zadd");
				ar.push_back(tab);
				ar.push_back(score);
				ar.push_back(key);
				return peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::ZDEL:
			{
				unsigned int len = (unsigned int)p[1]&0xff;
				lyramilk::data::string tab(1+p+1,len);
				unsigned int len2 = (unsigned int)p[1+1+len]&0xff;
				lyramilk::data::string key(1+p+1+len+1,len2);

				lyramilk::data::array ar;
				ar.reserve(3);
				ar.push_back("zrem");
				ar.push_back(tab);
				ar.push_back(key);
				return peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::QPUSH_BACK:
			{
				if(args.size() != 2){
					log(lyramilk::log::error,"psync") << D("同步错误:rpush 参数过少") << std::endl;
					break;
				}
				unsigned int len = (unsigned int)p[1]&0xff;
				lyramilk::data::string tab(1+p+1,len);
				/*
				lyramilk::data::string qseq(1+p+1+len,8);
				/*/
				unsigned long long qseq_net = *(unsigned long long*)(1+p+1+len);
				unsigned long long qseq = be64toh(qseq_net);
				if(qseq == QFRONT_SEQ || qseq == QBACK_SEQ){
					break;
				}
				/**/
				lyramilk::data::array ar;
				ar.reserve(3);
				ar.push_back("ssdb_qset");
				ar.push_back(tab);
				ar.push_back(qseq);
				ar.push_back(args[1]);
				return peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::QPUSH_FRONT:
			{
				if(args.size() != 2){
					log(lyramilk::log::error,"psync") << D("同步错误:lpush 参数过少") << std::endl;
					break; 
				}
				unsigned int len = (unsigned int)p[1]&0xff;
				lyramilk::data::string tab(1+p+1,len);
				/*
				lyramilk::data::string qseq(1+p+1+len,8);
				/*/
				unsigned long long qseq_net = *(unsigned long long*)(1+p+1+len);
				unsigned long long qseq = be64toh(qseq_net);
				if(qseq == QFRONT_SEQ || qseq == QBACK_SEQ){
					break;
				}
				/**/
				lyramilk::data::array ar;
				ar.reserve(3);
				ar.push_back("ssdb_qset");
				ar.push_back(tab);
				ar.push_back(qseq);
				ar.push_back(args[1]);
				return peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::QPOP_BACK:
			{
				lyramilk::data::string tab(p,l);
				lyramilk::data::array ar;
				ar.reserve(2);
				ar.push_back("rpop");
				ar.push_back(tab);
				return peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::QPOP_FRONT:
			{
				lyramilk::data::string tab(p,l);
				lyramilk::data::array ar;
				ar.reserve(2);
				ar.push_back("lpop");
				ar.push_back(tab);
				return peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::QSET:
			{
				if(args.size() != 2){
					log(lyramilk::log::error,"psync") << D("同步错误:lset 参数过少") << std::endl;
					break;
				}
				unsigned int len = (unsigned int)p[1]&0xff;
				lyramilk::data::string tab(1+p+1,len);
				/*
				lyramilk::data::string qseq(1+p+1+len,8);
				/*/
				unsigned long long qseq_net = *(unsigned long long*)(1+p+1+len);
				unsigned long long qseq = be64toh(qseq_net);
				if(qseq == QFRONT_SEQ || qseq == QBACK_SEQ){
					break;
				}
				/**/
				lyramilk::data::array ar;
				ar.reserve(4);
				ar.push_back("ssdb_qset");
				ar.push_back(tab);
				ar.push_back(qseq);
				ar.push_back(args[1]);
				return peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  default:
			log(lyramilk::log::warning,"proc_sync") << D("拷贝开始") << std::endl;
		}

		return true;
	}

}}
