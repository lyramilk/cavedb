#include "slave_ssdb.h"
#include "const.h"
#include <libmilk/multilanguage.h>
#include <libmilk/log.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

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
		active(2);
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
			is.init(c);
			if(!pwd.empty()){
				lyramilk::data::var::array ar;
				ar.push_back("auth");
				ar.push_back(pwd);
				push(ar);

				lyramilk::data::strings ret;
				pop(&ret);
				if(ret.size() == 1 && ret[0] == "OK") return true;
				return false;
			}
			return true;
		}
		return false;
	}

	bool slave_ssdb::exec(const lyramilk::data::var::array& cmd,lyramilk::data::strings* ret)
	{
		push(cmd);
		pop(ret);
		return true;
	}

	bool slave_ssdb::push(const lyramilk::data::var::array& cmd)
	{
		lyramilk::data::stringstream ss;
		{
			lyramilk::data::var::array::const_iterator it = cmd.begin();
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
			c.close();
			return false;
		}

/*
if(ret){
	lyramilk::data::strings::const_iterator it = ret->begin();
	COUT << "pop" << std::endl;
	for(;it!=ret->end();++it){
		COUT << *it << std::endl;
	}
}
*/

		return true;
	}

	int slave_ssdb::svc()
	{
		unsigned int tid = __sync_add_and_fetch(&loadsum,1);
		
		if(tid==1){
			while(status != st_stop){
				sleep(1);
			}
		}else{
			while(status != st_stop){
				if(!(is.good() && c.isalive())){
					reconnect();
					lyramilk::data::var::array ar;
					ar.push_back("sync140");
					ar.push_back(psync_offset);
					ar.push_back(psync_replid);
					ar.push_back("sync");
					push(ar);
				}
				if(c.check_read(800)){
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
							}
							break;
						  case BinlogType::COPY:
							proc_copy(seq,cmd,reply[0].c_str() + cmdoffset,reply[0].size() - cmdoffset,reply);
							break;
						  case BinlogType::SYNC:
						  case BinlogType::MIRROR:
							psync_offset = seq;
							proc_sync(seq,cmd,reply[0].c_str() + cmdoffset,reply[0].size() - cmdoffset,reply);
							break;
						}
					}
				}else{
					//log(lyramilk::log::warning,"psync") << D("负载%f",loadcoff) << std::endl;
					/*
					if(loadcoff * loadcoff * 10 < loadalive){
						psync_offset = is.rseq() + psync_rseq_diff;
						peventhandler->notify_idle(psync_replid,psync_offset);
						loadalive = 1;
					}*/
				}
			}
		}
		return 0;
	}

	void slave_ssdb::proc_noop(lyramilk::data::uint64 seq)
	{
		log(lyramilk::log::debug,"proc_noop") << D("空闲") << std::endl;
	}

	void slave_ssdb::proc_copy(lyramilk::data::uint64 seq,char cmd,const char* p,std::size_t l,const lyramilk::data::strings& args)
	{
		switch(cmd){
		  case BinlogCommand::BEGIN:
			log(lyramilk::log::debug,"proc_copy") << D("拷贝开始") << std::endl;
			{
				lyramilk::data::var::array ar;
				ar.push_back("flushall");
				peventhandler->notify_command(psync_replid,0,ar);
			}
			break;
		  case BinlogCommand::END:
			psync_replid = "";
			peventhandler->notify_psync(psync_replid,seq);
			log(lyramilk::log::debug,"proc_copy") << D("拷贝结束") << std::endl;
			break;
		  default:
			psync_replid.assign(p,l);
			proc_sync(seq,cmd,p,l,args);
		}
	}

	void slave_ssdb::proc_sync(lyramilk::data::uint64 seq,char cmd,const char* p,std::size_t l,const lyramilk::data::strings& args)
	{
		switch(cmd){
		  case BinlogCommand::KSET:
			{
				if(args.size() != 2){
					log(lyramilk::log::error,"psync") << D("同步错误:set 参数过少") << std::endl;
					break;
				}
				lyramilk::data::string tab(p+1,l-1);

				lyramilk::data::var::array ar;
				ar.push_back("set");
				ar.push_back(tab);
				ar.push_back(args[1]);
				peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::KDEL:
			{
				lyramilk::data::string tab(p+1,l-1);

				lyramilk::data::var::array ar;
				ar.push_back("del");
				ar.push_back(tab);
				peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::HSET:
			{
				if(args.size() != 2){
					log(lyramilk::log::error,"psync") << D("同步错误:hset 参数过少") << std::endl;
					break;
				}
				unsigned int len = (unsigned int)p[1];
				lyramilk::data::string tab(1+p+1,len);
				lyramilk::data::string key(1+p+1+len+1,l-2-len-1);

				lyramilk::data::var::array ar;
				ar.push_back("hset");
				ar.push_back(tab);
				ar.push_back(key);
				ar.push_back(args[1]);
				peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::HDEL:
			{
				unsigned int len = (unsigned int)p[1];
				lyramilk::data::string tab(1+p+1,len);
				lyramilk::data::string key(1+p+1+len+1,l-2-len-1);

				lyramilk::data::var::array ar;
				ar.push_back("hdel");
				ar.push_back(tab);
				ar.push_back(key);
				peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::ZSET:
			{
				if(args.size() != 2){
					log(lyramilk::log::error,"psync") << D("同步错误:zadd 参数过少") << std::endl;
					break;
				}

				unsigned int len = (unsigned int)p[1];
				lyramilk::data::string tab(1+p+1,len);
				unsigned int len2 = (unsigned int)p[1+1+len];
				lyramilk::data::string key(1+p+1+len+1,len2);

				lyramilk::data::var::array ar;
				ar.push_back("zadd");
				ar.push_back(tab);
				ar.push_back(args[1]);
				ar.push_back(key);
				peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::ZDEL:
			{
				unsigned int len = (unsigned int)p[1];
				lyramilk::data::string tab(1+p+1,len);
				unsigned int len2 = (unsigned int)p[1+1+len];
				lyramilk::data::string key(1+p+1+len+1,len2);

				lyramilk::data::var::array ar;
				ar.push_back("zrem");
				ar.push_back(tab);
				ar.push_back(key);
				peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::QPUSH_BACK:
			{
				if(args.size() != 2){
					log(lyramilk::log::error,"psync") << D("同步错误:lpush 参数过少") << std::endl;
					break;
				}
				unsigned int len = (unsigned int)p[1];
				lyramilk::data::string tab(1+p+1,len);
				lyramilk::data::var::array ar;
				ar.push_back("rpush");
				ar.push_back(tab);
				ar.push_back(args[1]);
				peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::QPUSH_FRONT:
			{
				if(args.size() != 2){
					log(lyramilk::log::error,"psync") << D("同步错误:lpush 参数过少") << std::endl;
					break;
				}
				unsigned int len = (unsigned int)p[1];
				lyramilk::data::string tab(1+p+1,len);
				lyramilk::data::var::array ar;
				ar.push_back("lpush");
				ar.push_back(tab);
				ar.push_back(args[1]);
				peventhandler->notify_command(psync_replid,psync_offset,ar);
			}
			break;
		  case BinlogCommand::QPOP_BACK:
			{
				//TODO();
			}
			break;
		  case BinlogCommand::QPOP_FRONT:
			{
				//TODO();
			}
			break;
		  case BinlogCommand::QSET:
			{
				//TODO();
			}
			break;
		  default:
			log(lyramilk::log::warning,"proc_sync") << D("拷贝开始") << std::endl;
		}
	}

}}