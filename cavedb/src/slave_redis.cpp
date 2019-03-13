#include "slave_redis.h"
#include "rdb.h"
#include <libmilk/dict.h>
#include <libmilk/log.h>
#include <libmilk/exception.h>
#include <fstream>
#include <stdlib.h>
#include <algorithm>
#include <unistd.h>

namespace lyramilk{ namespace cave
{
	static lyramilk::log::logss log("lyramilk.cave.slave_redis");

	bool inline parse_redis(std::istream& is,lyramilk::data::var& v)
	{
		char c = 0;
		if(is.get(c)){
			switch(c){
			  case '*':{
					lyramilk::data::string slen;
					slen.reserve(256);
					while(is.get(c)){
						if(c == '\r') continue;
						if(c == '\n') break;
						slen.push_back(c);
					}

					lyramilk::data::var len = slen;
					int ilen = len;
					v.type(lyramilk::data::var::t_array);

					lyramilk::data::array& ar = v;
					ar.resize(ilen);

					lyramilk::data::var* e = ar.data();
					for(int i=0;i<ilen;++i,++e){
						parse_redis(is,*e);
					}
					return true;
				}
				break;
			  case '$':{
					lyramilk::data::string slen;
					slen.reserve(256);
					while(is.get(c)){
						if(c == '\r') continue;
						if(c == '\n') break;
						slen.push_back(c);
					}
					if(slen == "-1") return true;
					lyramilk::data::var len = slen;
					int ilen = len;
					lyramilk::data::string buf;
					buf.resize(ilen);
					is.read((char*)buf.c_str(),ilen);
					buf.erase(buf.begin() + is.gcount(),buf.end());
					v = buf;
					char buff[2];
					is.read(buff,2);
					return true;
				}
				break;
			  case '+':{
					lyramilk::data::string str;
					str.reserve(4096);
					while(is.get(c)){
						if(c == '\r') continue;
						if(c == '\n') break;
						str.push_back(c);
					}
					v = str;
					return true;
				}
				break;
			  case '-':{
					lyramilk::data::string str;
					str.reserve(4096);
					while(is.get(c)){
						if(c == '\r') continue;
						if(c == '\n') break;
						str.push_back(c);
					}
					v = str;
					return false;
				}
				break;
			  case ':':{
					lyramilk::data::string str;
					str.reserve(4096);
					while(is.get(c)){
						if(c == '\r') continue;
						if(c == '\n') break;
						str.push_back(c);
					}

					/* 断言这个数字恒为非负整数，如果不是需要修改代码。 */
					v = str;
					v.type(lyramilk::data::var::t_int);
					return true;
				}
				break;
			  //default:
				//throw lyramilk::exception(D("redis 错误：响应格式错误%d",(unsigned int)c));
			}
		}
		log(lyramilk::log::error,"parse") << D("redis 错误：响应格式错误%u",(unsigned int)c) << std::endl;
		//throw lyramilk::exception(D("redis 错误：响应格式错误%d",(unsigned int)c));
		return false;
	}

	slave_redis::slave_redis()
	{
		status = st_idle;
		psync_replid = "?";
		psync_offset = 0;
		psync_rseq_diff = 0;
		loadalive = 1;
		loadcoff = 1;
		loadsum = 0;
	}

	slave_redis::~slave_redis()
	{
	}

	lyramilk::data::uint64 slave_redis::tell_offset()
	{
		return is.rseq() + psync_rseq_diff;
	}

	void slave_redis::slaveof(const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& pwd,lyramilk::data::string psync_replid,lyramilk::data::uint64 psync_offset,slave* peventhandler)
	{
		this->host = host;
		this->port = port;
		this->pwd = pwd;
		this->peventhandler = peventhandler;
		this->psync_replid = psync_replid;
		this->psync_rseq_diff = psync_rseq_diff;
		active(2);
	}

	bool slave_redis::reconnect()
	{
		c.close();
		if(c.open(host,port)){
			is.init(&c);
			if(!pwd.empty()){
				lyramilk::data::array ar;
				ar.push_back("auth");
				ar.push_back(pwd);
				push(ar);

				lyramilk::data::var ret;
				pop(&ret);
				return ret == "OK";
			}
			return true;
		}
		return false;
	}

	bool slave_redis::exec(const lyramilk::data::array& cmd,lyramilk::data::var* ret)
	{
		push(cmd);
		pop(ret);
		return true;
	}

	bool slave_redis::push(const lyramilk::data::array& cmd)
	{
		lyramilk::data::stringstream ss;
		{
			lyramilk::data::array::const_iterator it = cmd.begin();
			ss << "*" << cmd.size() << "\r\n";
			for(;it!=cmd.end();++it){
				lyramilk::data::string str = it->str();
				ss << "$" << str.size() << "\r\n";
				ss << str << "\r\n";
			}
			ss.flush();
		}
		lyramilk::data::string str = ss.str();
		return c.write(str.c_str(),str.size()) == (int)str.size();
	}

	bool slave_redis::pop(lyramilk::data::var* ret)
	{
		if(!parse_redis(is,*ret)){
			c.close();
			return false;
		}
		return true;
	}


	class myrdb:public rdb
	{
		slave* ps;
	  public:
		myrdb(slave* s)
		{
			ps = s;
		}
		virtual ~myrdb()
		{
		}

		virtual bool notify_select(lyramilk::data::uint64 dbid)
		{
			lyramilk::data::array ar;
			ar.push_back("select");
			ar.push_back(dbid);
			return ps->notify_command("?",0,ar);
		}

		virtual bool notify_aux(const lyramilk::data::string& key,const lyramilk::data::var& value)
		{
			return true;
		}

		virtual bool notify_hset(const lyramilk::data::string& key,const lyramilk::data::string& field,const lyramilk::data::var& value)
		{
			lyramilk::data::array ar;
			ar.push_back("hset");
			ar.push_back(key);
			ar.push_back(field);
			ar.push_back(value);
			return ps->notify_command("?",0,ar);
		}
		virtual bool notify_zadd(const lyramilk::data::string& key,const lyramilk::data::var& value,double score)
		{
			lyramilk::data::array ar;
			ar.push_back("zadd");
			ar.push_back(key);
			ar.push_back(score);
			ar.push_back(value);
			return ps->notify_command("?",0,ar);
		}
		virtual bool notify_set(const lyramilk::data::string& key,const lyramilk::data::string& value)
		{
			lyramilk::data::array ar;
			ar.push_back("set");
			ar.push_back(key);
			ar.push_back(value);
			return ps->notify_command("?",0,ar);
		}

		virtual bool notify_rpush(const lyramilk::data::string& key,const lyramilk::data::string& item)
		{
			lyramilk::data::array ar;
			ar.push_back("rpush");
			ar.push_back(key);
			ar.push_back(item);
			return ps->notify_command("?",0,ar);
		}

		virtual bool notify_sadd(const lyramilk::data::string& key,const lyramilk::data::string& value)
		{
			lyramilk::data::array ar;
			ar.push_back("sadd");
			ar.push_back(key);
			ar.push_back(value);
			return ps->notify_command("?",0,ar);
		}

		virtual bool notify_pexpireat(const lyramilk::data::string& key,lyramilk::data::uint64 expiretime)
		{
			lyramilk::data::array ar;
			ar.push_back("pexpireat");
			ar.push_back(key);
			ar.push_back(expiretime);
			return ps->notify_command("?",0,ar);
		}
	};

	int slave_redis::svc()
	{
		time_t last = time(nullptr);
		unsigned int tid = __sync_add_and_fetch(&loadsum,1);
		
		if(tid==1){
			unsigned int tmp = 0;
			while(status != st_stop){
				++tmp;
				sleep(1);
				if(tmp%3 == 0){
					lyramilk::data::array ar;
					ar.push_back("replconf");
					ar.push_back("ack");
					ar.push_back(psync_offset);
					push(ar);
				}
				if(tmp%10 == 0){
					lyramilk::data::array ar;
					ar.push_back("ping");
					push(ar);
				}
			}
		}else{
			while(status != st_stop){
				if(!(is.good() && c.isalive())){
					reconnect();
					{
						lyramilk::data::array ar;
						ar.push_back("replconf");
						ar.push_back("listening-port");
						ar.push_back("-1");
						push(ar);
						lyramilk::data::var ret;
						pop(&ret);
					}
					{
						lyramilk::data::array ar;
						ar.push_back("client");
						ar.push_back("setname");
						ar.push_back("cavedb");
						push(ar);
						lyramilk::data::var ret;
						pop(&ret);
					}
					{
						lyramilk::data::array ar;
						ar.push_back("psync");
						ar.push_back(psync_replid);
						ar.push_back(psync_offset + 1);	//psync_offset代表己验证的数据，所以psync时需要请求下一个字节
						push(ar);
						lyramilk::data::var ret;
						pop(&ret);
							
						lyramilk::data::string str = ret.str();
						lyramilk::data::string str2 = str;
						std::transform(str2.begin(),str2.end(),str2.begin(),::toupper);

						if(str2.compare(0,10,"FULLRESYNC",10) == 0){
							std::size_t sep2 = str.find_first_of(' ',11);
							if(sep2 != str.npos){
								psync_replid = str.substr(11,sep2-11);
							}
							char* p;
							psync_offset = strtoll(str.c_str() + sep2 + 1,&p,10);
							log(lyramilk::log::debug,"psync") << D("完全同步:masterid=%s,offset=%llu",psync_replid.c_str(),psync_offset) << std::endl;
							while(is.get() != '$');
							while(is.get() != '\n');

							{
								lyramilk::data::array ar;
								ar.push_back("flushall");
								peventhandler->notify_command("?",0,ar);
							}
							myrdb r(peventhandler);
							r.init(is);
							if(!peventhandler->notify_psync(psync_replid,psync_offset)){
								log(lyramilk::log::error,"psync") << D("从redis同步时保存psync数据发生错误") << std::endl;
							}
							psync_rseq_diff = psync_offset - is.rseq();
							log(lyramilk::log::debug,"psync") << D("完全完成:masterid=%s,offset=%llu",psync_replid.c_str(),psync_offset) << std::endl;
						}else if(str.compare(0,8,"CONTINUE",8) == 0){
							psync_rseq_diff = psync_offset - is.rseq();
							log(lyramilk::log::debug,"psync") << D("继续同步:masterid=%s,offset=%llu",psync_replid.c_str(),psync_offset) << std::endl;
						}

					}
				}

				time_t now = time(nullptr);
				if(last != now && psync_rseq_diff){
					++loadalive;
					loadcoff += loadsum;
					loadcoff *= 0.4;
					loadsum = 0;


					last = now;
				}

				if(c.check_read(800)){
					lyramilk::data::var ret;
					pop(&ret);
					if(ret.type() == lyramilk::data::var::t_array){
						++loadsum;
						lyramilk::data::array& ar = ret;
						if(ar.size() > 0){
							lyramilk::data::string cmd = ar[0];
							std::transform(cmd.begin(),cmd.end(),cmd.begin(),::tolower);
							ar[0] = cmd;
							psync_offset = is.rseq() + psync_rseq_diff;
							peventhandler->notify_command(psync_replid,psync_offset,ar);
						}
					}else{
						if(ret.type() == lyramilk::data::var::t_str && ret.str() != "PONG"){
							log(lyramilk::log::warning,"psync") << D("从redis同步时发现未知的返回类型%s\n",ret.type_name().c_str()) << std::endl;
						}
					}
				}else{
					//log(lyramilk::log::warning,"psync") << D("负载%f",loadcoff) << std::endl;
					if(loadcoff * loadcoff * 10 < loadalive){
						psync_offset = is.rseq() + psync_rseq_diff;
						peventhandler->notify_idle(psync_replid,psync_offset);
						loadalive = 1;
					}
				}
			}
		}
		return 0;
	}
}}
