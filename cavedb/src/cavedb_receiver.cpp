#include "cavedb_receiver.h"
#include "leveldb_store.h"

#include <libmilk/log.h>
#include <libmilk/dict.h>
#include <unistd.h>

#include <libmilk/json.h>

/*
#include "ssdb/const.h"
#include <libmilk/dict.h>
#include <libmilk/log.h>
#include <libmilk/exception.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <endian.h>
#include <unistd.h>


const uint64_t QFRONT_SEQ = 2;
const uint64_t QBACK_SEQ  = 3;
const uint64_t QITEM_MIN_SEQ = 10000;
const uint64_t QITEM_MAX_SEQ = 9223372036854775807ULL;
const uint64_t QITEM_SEQ_INIT = QITEM_MAX_SEQ/2;
*/

namespace lyramilk{ namespace cave
{

	static lyramilk::log::logss log("lyramilk.cave.cavedb_receiver");


	cavedb_receiver::cavedb_receiver()
	{
		status = st_idle;
		psync_replid = "";
		psync_offset = 0;
	}

	cavedb_receiver::~cavedb_receiver()
	{
		status = st_stop;
	}


	lyramilk::data::uint64 cavedb_receiver::tell_offset()
	{
		return psync_offset;
	}

	void cavedb_receiver::init(const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& pwd,const lyramilk::data::string& masterid,lyramilk::data::string psync_replid,lyramilk::data::uint64 psync_offset,cmd_accepter* cmdr)
	{
		this->host = host;
		this->port = port;
		this->masterauth = pwd;
		this->cmdr = cmdr;
		this->psync_replid = psync_replid;
		this->psync_offset = psync_offset;
		this->masterid = masterid;
	}




	bool cavedb_receiver::sync_once(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::uint64 count,lyramilk::data::array* cmds,lyramilk::data::string* nextreplid,lyramilk::data::uint64* nextseq)
	{
		if(!(is.good() && c.isalive())){
			is.init(&c);
			if(!reconnect()) {
				sleep(3);
				return false;
			}
		}

		lyramilk::data::stringstream oss;
		oss << "*4\r\n";
		oss << "$9\r\ncave_sync\r\n";
		oss << "$" << replid.size() << "\r\n" << replid << "\r\n";
		oss << ":" << offset << "\r\n";
		oss << ":" << count << "\r\n";
		lyramilk::data::string str = oss.str();
		c.write(str.c_str(),str.size());

		lyramilk::data::var v;
		if(resp23_from_stream(is,&v) != resp_data) return false;
		if(v.type() == lyramilk::data::var::t_array){
			lyramilk::data::array& ar = v;
			if(ar.size() != 3) return false;

			*nextreplid = ar[0].str();
			*nextseq = ar[1].conv(0);
			if(ar[2].type() != lyramilk::data::var::t_array) return false;
			lyramilk::data::array& subar = ar[2];
			cmds->swap(subar);
		}

		return true;
	}

	bool cavedb_receiver::reconnect()
	{
		c.close();
		if(c.open(host,port)){
			c.setnodelay(true);
			if(!masterauth.empty()){
				lyramilk::data::stringstream oss;
				oss << "*2\r\n";
				oss << "$4\r\nauth\r\n";
				oss << "$" << masterauth.size() << "\r\n" << masterauth << "\r\n";
				lyramilk::data::string str = oss.str();
				c.write(str.c_str(),str.size());

				lyramilk::netio::socket_istream is;
				is.init(&c);

				lyramilk::data::var v;
				resp_result rr = resp23_from_stream(is,&v);

				if(rr == resp_hidden_data){
					rr = resp23_from_stream(is,&v);
				}


				if(rr == resp_msg_error){
					lyramilk::data::string str = v.str();
					log(lyramilk::log::error,"psync") << D("[%s]同步出错:%s",masterid.c_str(),str.c_str()) << std::endl;
					return false;
				}
				if(rr == resp_parse_error){
					log(lyramilk::log::error,"psync") << D("[%s]同步出错:协议解析失败",masterid.c_str()) << std::endl;
					return false;
				}
				if(rr == resp_data){
					log(lyramilk::log::debug,"psync") << D("[%s]建立同步关系",masterid.c_str()) << std::endl;
					return true;
				}
				log(lyramilk::log::error,"psync") << D("[%s]同步出错:未知错误",masterid.c_str()) << std::endl;
				return false;
			}
			return true;
		}
		//log(lyramilk::log::error,"psync") << D("[%s]同步出错:连接失败",masterid.c_str()) << std::endl;
		return false;
	}

	int cavedb_receiver::svc()
	{
		lyramilk::data::string nextreplid = psync_replid;
		lyramilk::data::uint64 nextseq = psync_offset;
		while(status != st_stop){
			psync_replid = nextreplid;
			psync_offset = nextseq;

			lyramilk::data::array cmds;
			if(!sync_once(psync_replid,psync_offset,3000,&cmds,&nextreplid,&nextseq)){
				//log(lyramilk::log::error,"psync") << D("[%s]同步出错，重新链接",masterid.c_str()) << std::endl;
				c.close();
				sleep(2);
				continue;
			}

			for(lyramilk::data::array::const_iterator it = cmds.begin();it!=cmds.end();++it){
				if(it->type() != lyramilk::data::var::t_array) break;
				const lyramilk::data::array& ar = *it;
				lyramilk::data::var ret;
				if(lyramilk::cave::cmdstatus::cs_error == cmdr->call(masterid,psync_replid,psync_offset,ar,&ret,&chd,&sen)){
					lyramilk::data::string s = lyramilk::data::json::stringify(ar);
					log(lyramilk::log::error,"psync") << D("[%s]同步执行命令出错%.*s，重新链接",masterid.c_str(),s.size(),s.c_str()) << std::endl;
					c.close();
					sleep(2);
				}
			}

			if(cmds.empty()){
				sleep(1);
				continue;
			}
			cmdr->save_sync_info(masterid,nextreplid,nextseq);
		}
		log(lyramilk::log::error,"psync") << D("[%s]同步线程退出",masterid.c_str()) << std::endl;
		return 0;
	}


	cavedb_receiver::st_status cavedb_receiver::get_sync_status()
	{
		return status;
	}


}}

