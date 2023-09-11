#include "cmd_accepter.h"
#include "leveldb_store.h"
#include "binlog_store.h"
#include "command.h"
#include "resp.h"
#include "ssdb_receiver.h"
#include "cavedb_receiver.h"
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <fstream>
#include <sys/wait.h>
#include <libmilk/json.h>
#include <libmilk/log.h>
#include <libmilk/dict.h>
#include <libmilk/aio.h>
#include <libmilk/netio.h>


class CaveDBServerSession:public lyramilk::cave::resp23_as_session
{
  public:
	lyramilk::cave::cmd_accepter* cmdr;
	lyramilk::cave::cmdsessiondata sen;
	lyramilk::cave::cmdchanneldata* chd;
	lyramilk::data::string masterid;

	CaveDBServerSession()
	{
		sen.loginseq = 0;
	}

	virtual ~CaveDBServerSession()
	{
		
	}

	virtual bool oninit(lyramilk::data::ostream& os)
	{
		setnodelay(true);
	}


	virtual bool notify_cmd(const lyramilk::data::array& cmd, lyramilk::data::ostream& os)
	{
		lyramilk::data::var ret;
		lyramilk::cave::cmdstatus rs = cmdr->call(masterid,"",0,cmd,&ret,chd,&sen);
		return output_redis_result(rs,ret,os);
	}
};



class CaveDBServer:public lyramilk::netio::aioserver<CaveDBServerSession>
{
  public:
	lyramilk::cave::cmd_accepter* cmdr;
	lyramilk::cave::cmdchanneldata chd;

	lyramilk::data::string masterid;

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
		p->cmdr = cmdr;
		p->chd = &chd;

		p->masterid = masterid;
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
	bool ondaemon = false;
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


	lyramilk::data::var v;

	{
		//后台模式加载配置文件
		std::ifstream ifs;
		ifs.open(configfile);
		if(!ifs){
			lyramilk::klog(lyramilk::log::error,"cavedb") << "打开配置文件失败" << std::endl;
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

		v.clear();
		v = lyramilk::data::json::parse(configjson);
		if(v.type() != lyramilk::data::var::t_map){
			lyramilk::klog(lyramilk::log::error,"cavedb") << "解析配置文件失败" << std::endl;
			return -1;
		}
	}
	if(isdaemon){
		ondaemon = ::daemon(1,0) == 0;
	}

	lyramilk::data::map cfobj = v;


	{
		lyramilk::data::string logfile;
		if(cfobj["logfile"].type() == lyramilk::data::var::t_str){
			logfile = cfobj["logfile"].str();
			if(ondaemon){
				lyramilk::log::logf* lf = new lyramilk::log::logf(logfile);
				lyramilk::klog.rebase(lf);
			}else{
				lyramilk::log::logfc* lf = new lyramilk::log::logfc(logfile);
				lyramilk::klog.rebase(lf);
			}
		}else{
			lyramilk::klog(lyramilk::log::warning,"cavedb") << "解析配置文件失败: logfile 类型不对，应该是t_str。日志将通过默认方式输出。" << std::endl;
		}
	}



	if(cfobj["store"].type() != lyramilk::data::var::t_map){
		lyramilk::klog(lyramilk::log::error,"cavedb") << "解析配置文件失败: store 类型不对，应该是t_map" << std::endl;
		return -1;
	}

	lyramilk::data::map gstore = cfobj["store"];

	if(gstore["type"].type() == lyramilk::data::var::t_str){
		if(gstore["type"].str() != "leveldb"){
			lyramilk::klog(lyramilk::log::error,"cavedb") << "解析配置文件失败: store.type仅支持leveldb2" << std::endl;
			return -1;
		}
	}

	
	if(cfobj["slaveof"].type() != lyramilk::data::var::t_array){
		lyramilk::klog(lyramilk::log::error,"cavedb") << "解析配置文件失败: slaveof 类型不对，应该是t_array" << std::endl;
		return -1;
	}
	lyramilk::data::array slaveof;
	lyramilk::data::array& tslaveof = cfobj["slaveof"];
	for(lyramilk::data::array::iterator it = tslaveof.begin();it!=tslaveof.end();++it){
		if(it->type() == lyramilk::data::var::t_map){
			lyramilk::data::map& m = *it;
			if(m["type"].type()!= lyramilk::data::var::t_str){
				lyramilk::klog(lyramilk::log::error,"cavedb") << "解析配置文件失败: slaveof 类型子对象的type类型不对，应该是t_str" << std::endl;
				return -1;
			}
			slaveof.push_back(*it);
		}
	}

	if(cfobj["server"].type() != lyramilk::data::var::t_array){
		lyramilk::klog(lyramilk::log::error,"cavedb") << "解析配置文件失败: server 类型不对，应该是t_array" << std::endl;
		return -1;
	}
	lyramilk::data::array server;
	lyramilk::data::array& tserver = cfobj["server"];
	for(lyramilk::data::array::iterator it = tserver.begin();it!=tserver.end();++it){
		if(it->type() == lyramilk::data::var::t_map){
			lyramilk::data::map& m = *it;
			if(m["type"].type()!= lyramilk::data::var::t_str){
				lyramilk::klog(lyramilk::log::error,"cavedb") << "解析配置文件失败: server 类型子对象的type类型不对，应该是t_str" << std::endl;
				return -1;
			}
			server.push_back(*it);
		}
	}

	signal(SIGPIPE, SIG_IGN);

	// 初始化epoll
	lyramilk::io::aiopoll pool;
	pool.active(4);




	lyramilk::cave::leveldb_store cmdr;
	if(cmdr.open_leveldb(gstore["path"].str(),(unsigned int)gstore["cache"].conv(500),true)){
		/*
		lyramilk::data::string str = gstore["requirepass"].str();
		cmdr.set_requirepass(str);
		*/
	}else{
		lyramilk::klog(lyramilk::log::error,"cavedb") << "打开leveldb失败:" << gstore << std::endl;
		return -1;
	}

	if(cfobj.find("binlog") != cfobj.end()){
		if(cfobj["binlog"].type() != lyramilk::data::var::t_map){
			lyramilk::klog(lyramilk::log::error,"cavedb") << "解析配置文件失败: binlog.type必须是字典类型" << std::endl;
			return -1;
		}

		lyramilk::data::map& binlog = cfobj["binlog"];
		lyramilk::data::string binlogtype = "leveldb";
		if(binlog["type"].type() == lyramilk::data::var::t_str){
			binlogtype = binlog["type"].str();
			if(binlog["type"].str() != "leveldb"){
				lyramilk::klog(lyramilk::log::error,"cavedb") << "解析配置文件失败: binlog.type仅支持leveldb" << std::endl;
				return -1;
			}
		}


		if(binlogtype == "leveldb"){
			lyramilk::cave::binlog_leveldb* blog = new lyramilk::cave::binlog_leveldb;
			if(!blog->open_leveldb(binlog["path"].str(),(unsigned int)binlog["cache"].conv(500),true,(unsigned long long)binlog["capacity"].conv(20000000))){
				lyramilk::klog(lyramilk::log::error,"cavedb") << "打开leveldb失败:" << binlog << std::endl;
				return -1;
			}
			cmdr.set_binlog(blog);
		}else if (binlogtype == "file"){
#if 0
			lyramilk::cave::binlog_appendfiles* blog = new lyramilk::cave::binlog_appendfiles;
			if(!blog->open_path(binlog["path"].str(),(unsigned long long)binlog["capacity"].conv(20000000))){
				lyramilk::klog(lyramilk::log::error,"cavedb") << "打开leveldb失败:" << binlog << std::endl;
				return -1;
			}
			cmdr->set_binlog(&blog);
#endif
		}
	}

	//	初始化主从同步的主库。
	for(lyramilk::data::array::iterator it = slaveof.begin();it!=slaveof.end();++it){
		if(it->type() == lyramilk::data::var::t_map){
			lyramilk::data::map& m = *it;
			lyramilk::data::string type = m["type"].str();
			if(type == "ssdb" && false){
				lyramilk::cave::ssdb_receiver* p = new lyramilk::cave::ssdb_receiver;
				p->chd.isreadonly = false;

				lyramilk::data::string host = m["host"].str();
				lyramilk::data::int32 port = m["port"].conv(-1);
				lyramilk::data::string masterauth = m["masterauth"].str();
				lyramilk::data::string masterid = m["masterid"].str();

				lyramilk::data::string replid = "";
				lyramilk::data::uint64 offset = 0;
				cmdr.get_sync_info(masterid,&replid,&offset);

				p->init(host,port,masterauth,masterid,replid,offset,&cmdr);
				p->active(1);
			}

			if(type == "cavedb"){
				lyramilk::cave::cavedb_receiver* p = new lyramilk::cave::cavedb_receiver;
				p->chd.isreadonly = false;

				lyramilk::data::string host = m["host"].str();
				lyramilk::data::int32 port = m["port"].conv(-1);
				lyramilk::data::string masterauth = m["masterauth"].str();
				lyramilk::data::string masterid = m["masterid"].str();

				lyramilk::data::string replid = "";
				lyramilk::data::uint64 offset = 0;
				cmdr.get_sync_info(masterid,&replid,&offset);


				p->init(host,port,masterauth,masterid,replid,offset,&cmdr);
				p->active(1);
			}
		}
	}

	//	初始化对外服务。
	for(lyramilk::data::array::iterator it = server.begin();it!=server.end();++it){
		if(it->type() == lyramilk::data::var::t_map){
			lyramilk::data::map& m = *it;
			lyramilk::data::string type = m["type"].str();
			if(type == "network"){
				CaveDBServer* p = new CaveDBServer;
				p->cmdr = &cmdr;

				lyramilk::data::string server_host = m["host"].str();
				lyramilk::data::int32 server_port = m["port"].conv(-1);
				p->masterid = m["masterid"].str();
				p->chd.requirepass = m["requirepass"].str();
				p->chd.isreadonly = m["readonly"].conv(false);

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
				p->cmdr = &cmdr;
				p->masterid = m["masterid"].str();
				p->chd.requirepass = m["requirepass"].str();
				p->chd.isreadonly = m["readonly"].conv(false);

				if(!p->open_unixsocket(m["unix"].str())){
					lyramilk::klog(lyramilk::log::error,"cavedb") << "unixsocket打开失败：" << m["unix"].str() << std::endl;
					return -1;
				}
				pool.add(p);
			}
		}
	}

	while(true){
		if(cmdr.rspeed || cmdr.wspeed){
			lyramilk::klog(lyramilk::log::debug,"cavedb") << "读取速度:" << cmdr.rspeed << ",写入速度:" << cmdr.wspeed << std::endl;
		}
		sleep(1);
	}

	return 0;
}


/*
nct:1,age
*/