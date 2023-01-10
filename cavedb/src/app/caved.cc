#include "cmdlistener.h"
#include "command.h"
#include "resp.h"
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <fstream>
#include <libmilk/json.h>
#include <libmilk/log.h>
#include <libmilk/dict.h>
#include <libmilk/aio.h>
#include <libmilk/netio.h>


class CaveDBServerSession:public lyramilk::cave::resp23_as_session
{
  public:
	lyramilk::cave::cmdlistener* cmdr;
	lyramilk::cave::cmdsessiondata sen;

	CaveDBServerSession()
	{
		sen.loginseq = 0;
	}

	virtual ~CaveDBServerSession()
	{
		
	}

	virtual bool notify_cmd(const lyramilk::data::array& cmd, lyramilk::data::ostream& os)
	{
		lyramilk::data::var ret;
		lyramilk::cave::cmdstatus rs = cmdr->call(cmd,&ret,&sen);
		return output_redis_result(rs,ret,os);
	}
};



class CaveDBServer:public lyramilk::netio::aioserver<CaveDBServerSession>
{
  public:
	lyramilk::cave::cmdlistener cmdr;

	//lyramilk::cave::leveldb_standard* store;
	lyramilk::data::string requirepass;
	bool readonly;

	CaveDBServer()
	{
		//store = nullptr;
		//cmdr.init();
	}

	virtual ~CaveDBServer()
	{
	
	}
	virtual lyramilk::netio::aiosession* create()
	{
		CaveDBServerSession *p = lyramilk::netio::aiosession::__tbuilder<CaveDBServerSession>();
		p->cmdr = &cmdr;
		//p->init_cavedb("cavedb",requirepass,store,false);
		return p;
	}
};



void useage(lyramilk::data::string selfname)
{
	std::cout << "useage:" << selfname << " [optional]" << std::endl;
	std::cout << "\t-d --daemon                                  \t" << "以服务方式启动" << std::endl;
	std::cout << "\t-c --config-file   < config file >           \t" << "ssdb主库的host" << std::endl;
	std::cout << "\t-? --help                                    \t显示这个帮助" << std::endl;
}

struct option long_options[] = {
	{ "daemon", required_argument, NULL, 'd' },
	{ "config-file", required_argument, NULL, 'c' },
	{ "help", required_argument, NULL, '?' },
	{ 0, 0, 0, 0},
};

int main(int argc,char* argv[])
{
	bool isdaemon = false;
	lyramilk::data::string configfile;
	lyramilk::data::string selfname = argv[0];
	if(argc > 1){
		int oc;
		while((oc = getopt_long(argc, argv, "dc:?",long_options,nullptr)) != -1){
			switch(oc)
			{
			  case 'd':
				isdaemon = true;
				break;
			  case 'c':
				configfile = optarg;
				break;
			  case '?':
			  default:
				useage(selfname);
				return 0;
			}
		}
	}else{
		useage(selfname);
		return 0;
	}

	if(isdaemon){
		daemon(0,0);
	}



	lyramilk::data::string logfile;
	lyramilk::data::map gstore;
	lyramilk::data::array gsource;
	lyramilk::data::array gserver;

	{
		errno = 0;
		std::ifstream ifs;
		ifs.open(configfile);
		if(!ifs){
			perror("打开配置文件失败");
			return -1;
		}

		lyramilk::data::string configjson;
		while(ifs){
			char buff[1024];
			ifs.read(buff,sizeof(buff));
			if(ifs.gcount() > 0){
				configjson.append(buff,ifs.gcount());
			}
		}


		lyramilk::data::var v = lyramilk::data::json::parse(configjson);
		if(v.type() != lyramilk::data::var::t_map){
			perror("解析配置文件失败");
			return -1;
		}

		lyramilk::data::map cfobj = v;

		if(cfobj["logfile"].type() == lyramilk::data::var::t_str){
			logfile = cfobj["logfile"].str();
			if(isdaemon){
				lyramilk::log::logf* lf = new lyramilk::log::logf(logfile);
				lyramilk::klog.rebase(lf);
			}else{
				lyramilk::log::logfc* lf = new lyramilk::log::logfc(logfile);
				lyramilk::klog.rebase(lf);
			}
		}else{
			lyramilk::klog(lyramilk::log::warning,"cavedb") << "解析配置文件失败: logfile 类型不对，应该是t_str。日志将通过默认方式输出。" << std::endl;
		}

		if(cfobj["store"].type() != lyramilk::data::var::t_map){
			perror("解析配置文件失败: store 类型不对，应该是t_map");
			return -1;
		}

		if(cfobj["source"].type() != lyramilk::data::var::t_array){
			perror("解析配置文件失败: source 类型不对，应该是t_array");
			return -1;
		}

		if(cfobj["server"].type() != lyramilk::data::var::t_array){
			perror("解析配置文件失败: server 类型不对，应该是t_array");
			return -1;
		}



		gstore = cfobj["store"];

		if(gstore["type"].type() == lyramilk::data::var::t_str){
			if(gstore["type"].str() != "leveldb2"){
				perror("解析配置文件失败: store.type仅支持leveldb2");
				return -1;
			}
		}

		lyramilk::data::array& source = cfobj["source"];
		lyramilk::data::array& server = cfobj["server"];


		for(lyramilk::data::array::iterator it = source.begin();it!=source.end();++it){
			if(it->type() == lyramilk::data::var::t_map){
				lyramilk::data::map& m = *it;
				if(m["type"].type()!= lyramilk::data::var::t_str){
					perror("解析配置文件失败: source 类型子对象的type类型不对，应该是t_str");
					return -1;
				}
				gsource.push_back(*it);
			}
		}

		for(lyramilk::data::array::iterator it = server.begin();it!=server.end();++it){
			if(it->type() == lyramilk::data::var::t_map){
				lyramilk::data::map& m = *it;
				if(m["type"].type()!= lyramilk::data::var::t_str){
					perror("解析配置文件失败: source 类型子对象的type类型不对，应该是t_str");
					return -1;
				}
				gserver.push_back(*it);
			}
		}
	}

	signal(SIGPIPE, SIG_IGN);

	// 初始化epoll
	lyramilk::io::aiopoll pool;
	pool.active(10);


/*
	//	初始化主库。
	for(lyramilk::data::array::iterator it = gsource.begin();it!=gsource.end();++it){
		if(it->type() == lyramilk::data::var::t_map){
			lyramilk::data::map& m = *it;
			lyramilk::data::string type = m["type"].str();
			if(type == "ssdb"){
				lyramilk::cave::slave_ssdb* datasource = new lyramilk::cave::slave_ssdb;

				lyramilk::data::string host = m["host"].str();
				lyramilk::data::int32 port = m["port"].conv(-1);
				lyramilk::data::string password = m["password"].str();
				lyramilk::data::string masterid = m["masterid"].str();

				lyramilk::data::string replid = "";
				lyramilk::data::uint64 offset = 0;
				mstore->get_sync_info(masterid,&replid,&offset);

				datasource->slaveof(host,port,password,masterid,replid,offset,mstore);
				lyramilk::klog(lyramilk::log::debug,"cavedb") << "同步于ssdb:" << host  << ":" << port << ",replid=" << replid << ",offset=" << offset << std::endl;
			}else if(type == "redis"){
				lyramilk::cave::slave_redis* datasource = new lyramilk::cave::slave_redis;

				lyramilk::data::string host = m["host"].str();
				lyramilk::data::int32 port = m["port"].conv(-1);
				lyramilk::data::string password = m["password"].str();
				lyramilk::data::string masterid = m["masterid"].str();

				lyramilk::data::string replid = "";
				lyramilk::data::uint64 offset = 0;
				mstore->get_sync_info(masterid,&replid,&offset);
#ifdef _DEBUG
				replid = "";
				offset = 0;
#endif
				datasource->slaveof(host,port,password,masterid,replid,offset,mstore);
				lyramilk::klog(lyramilk::log::debug,"cavedb") << "同步于redis:" << host  << ":" << port << ",replid=" << replid << ",offset=" << offset << std::endl;
			}else{
				lyramilk::klog(lyramilk::log::error,"cavedb") << "不支持的source类型：" << type << std::endl;
			}
		}
	}*/










	//	初始化对外服务。
	for(lyramilk::data::array::iterator it = gserver.begin();it!=gserver.end();++it){
		if(it->type() == lyramilk::data::var::t_map){
			lyramilk::data::map& m = *it;
			lyramilk::data::string type = m["type"].str();
			if(type == "network"){
				CaveDBServer* p = new CaveDBServer;
				//p->store = mstore;
				p->readonly = m["readonly"].conv(false);
				p->requirepass = m["password"].str();
				lyramilk::data::string server_host = m["host"].str();
				lyramilk::data::int32 server_port = m["port"].conv(-1);

				if(server_host == "0.0.0.0" || server_host.empty()){
					if(!p->open(server_port)){
						lyramilk::klog(lyramilk::log::error,"cavedb") << "socket打开失败：" << m["host"].str() << ":" << m["port"] << std::endl;
						return -1;
					}
				}else{
					if(!p->open(server_host,server_port)){
						lyramilk::klog(lyramilk::log::error,"cavedb") << "socket打开失败：" << m["host"].str() << ":" << m["port"] << std::endl;
						return -1;
					}
				}
				pool.add(p);
			}else if(type == "unixsocket"){
				CaveDBServer* p = new CaveDBServer;
				//p->store = mstore;
				p->readonly = m["readonly"].conv(false);
				p->requirepass = m["password"].str();
				if(!p->open_unixsocket(m["unix"].str())){
					lyramilk::klog(lyramilk::log::error,"cavedb") << "unixsocket打开失败：" << m["unix"].str() << std::endl;
					return -1;
				}
				pool.add(p);
			}
		}
	}

	pool.svc();

	return 0;
}