#include <unistd.h>
#include <libmilk/json.h>
#include <libmilk/log.h>
#include <libmilk/dict.h>
#include <libmilk/aio.h>
#include <libmilk/hash.h>
#include <libmilk/stringutil.h>
#include <getopt.h>
#include "config.h"
#include "store/leveldb_standard.h"
#include "slave_ssdb.h"
#include "redis_session.h"
#include <signal.h>
#include <fstream>
#include <algorithm>
#include <errno.h>

#include <sys/wait.h>

class TeapoyDBServer_session;

typedef lyramilk::cave::redis_session::result_status (TeapoyDBServer_session::*redis_cmd_callback)(const lyramilk::data::array& cmd, std::ostream& os);

struct redis_cmd_spec
{
	const static int write = 0x1;// - command may result in modifications
	const static int readonly = 0x2;// - command will never modify keys
	const static int denyoom = 0x4;// - reject command if currently OOM
	const static int admin = 0x8;// - server admin command
	const static int pubsub = 0x10;// - pubsub-related command
	const static int noscript =0x20;// - deny this command from scripts
	const static int random = 0x40;// - command has random results, dangerous for scripts
	const static int sort_for_script = 0x80;// - if called from script, sort output
	const static int loading = 0x100;// - allow command while database is loading
	const static int stale = 0x200;// - allow command while replica has stale data
	const static int skip_monitor = 0x400;// - do not show this command in MONITOR
	const static int asking = 0x800;// - cluster related - accept even if importing
	const static int fast = 0x1000;// - command operates in constant or log(N) time. Used for latency monitoring.
	const static int noauth = 0x2000;// 非redis状态，而是cavedb特有的，允许在非登录状态下可用这个，该命令不在command命令中返回。

	redis_cmd_callback c;

	int n;
	int f;
	int firstkey;
	int lastkey;
	int keystepcount;
};

std::map<lyramilk::data::string,redis_cmd_spec> dispatch;


class TeapoyDBServer_session:public lyramilk::netio::aiosession_sync,public lyramilk::cave::redis_session
{
  public:
	std::stringstream tcpcache;
	lyramilk::data::string requirepass;
	lyramilk::data::string pass;
	lyramilk::data::string masterid;
	bool multi;
	bool readonly;
	lyramilk::cave::leveldb_standard* store;
	std::map<lyramilk::data::uint64,lyramilk::data::string> scan_cursors;

	TeapoyDBServer_session()
	{
		multi = false;
		readonly = false;
	}

	virtual ~TeapoyDBServer_session()
	{
	}

	void static static_init_dispatch()
	{

		// def_cmd(命令,参数最少数量(如果是变长参数则为负数),标记,第一个key参数的序号,最后一个key参数的序号,重复参数的步长);
		#define def_cmd(cmd,ac,fg,fk,lk,kc) dispatch[#cmd].c = &TeapoyDBServer_session::notify_##cmd,dispatch[#cmd].f = (fg),dispatch[#cmd].firstkey = fk,dispatch[#cmd].lastkey = lk,dispatch[#cmd].keystepcount = kc,dispatch[#cmd].n = ac;
		def_cmd(hset,4,redis_cmd_spec::write,1,1,1);
		def_cmd(hmset,-4,redis_cmd_spec::write,1,1,1);
		def_cmd(hdel,-3,redis_cmd_spec::write,1,1,1);
		def_cmd(hget,3,redis_cmd_spec::readonly,1,1,1);
		def_cmd(hgetall,2,redis_cmd_spec::readonly,1,1,1);
		def_cmd(hscan,3,redis_cmd_spec::readonly,0,0,0);
		def_cmd(hlen,2,redis_cmd_spec::readonly,1,1,1);
		def_cmd(auth,2,redis_cmd_spec::readonly|redis_cmd_spec::loading|redis_cmd_spec::noauth,0,0,0);
		def_cmd(multi,1,redis_cmd_spec::readonly,0,0,0);
		def_cmd(exec,1,redis_cmd_spec::readonly,0,0,0);
		def_cmd(command,1,redis_cmd_spec::readonly|redis_cmd_spec::skip_monitor,0,0,0);
		def_cmd(info,1,redis_cmd_spec::readonly|redis_cmd_spec::skip_monitor,0,0,0);
		def_cmd(scan,2,redis_cmd_spec::readonly,0,0,0);
		def_cmd(type,2,redis_cmd_spec::readonly,1,1,1);
		def_cmd(set,3,redis_cmd_spec::write,1,1,1);
		def_cmd(get,2,redis_cmd_spec::readonly,1,1,1);
		def_cmd(zadd,4,redis_cmd_spec::write,1,1,1);
		def_cmd(zrem,3,redis_cmd_spec::write,1,1,1);
		def_cmd(zscan,3,redis_cmd_spec::readonly,1,1,1);
		def_cmd(zrange,-4,redis_cmd_spec::readonly,1,1,1);
		def_cmd(del,2,redis_cmd_spec::write,1,1,1);
		def_cmd(monitor,1,redis_cmd_spec::readonly|redis_cmd_spec::skip_monitor,1,1,1);
		def_cmd(ping,1,redis_cmd_spec::readonly|redis_cmd_spec::skip_monitor,1,1,1);
		#undef def_cmd
	}

	result_status notify_zadd(const lyramilk::data::array& cmd, std::ostream& os)
	{
		store->post_command(masterid,"",0,(lyramilk::data::array&)cmd,nullptr,false);
		os << ":1\r\n";
		return rs_ok;
	}

	result_status notify_zrem(const lyramilk::data::array& cmd, std::ostream& os)
	{
		store->post_command(masterid,"",0,(lyramilk::data::array&)cmd,nullptr,false);
		os << ":1\r\n";
		return rs_ok;
	}

	result_status notify_zscan(const lyramilk::data::array& cmd, std::ostream& os)
	{
		lyramilk::data::uint64 current_hash = cmd[2].conv(0ull);
		lyramilk::data::string cursor;
		if(current_hash != 0){
			std::map<lyramilk::data::uint64,lyramilk::data::string>::const_iterator it = rainbow_table.find(current_hash);
			if(it!=rainbow_table.end()){
				cursor = it->second;
			}else{
				os << "-ERR invalid cursor\r\n";
				return rs_ok;
			}
		}

		lyramilk::data::strings results;
		lyramilk::data::string next_cursor = store->zscan(cmd[1].str(),cursor,50,&results);

		os << "*2\r\n";
		if(next_cursor.empty()){
			os << "$1\r\n0\r\n";
		}else{
			lyramilk::data::uint64 nexthash = lyramilk::cryptology::hash64::fnv(next_cursor.data(),next_cursor.size());
			rainbow_table[nexthash] = next_cursor;

			lyramilk::data::string nexthashstr = lyramilk::data::str(nexthash);
			os << "$" << nexthashstr.size() << "\r\n";
			os << nexthashstr << "\r\n";
		}

		{
			os << "*" << results.size() << "\r\n";
			for(lyramilk::data::strings::iterator it = results.begin();it!=results.end();++it){
				os << "$" << it->size() << "\r\n";
				os << *it << "\r\n";
			}
		}
		return rs_ok;
	}

	result_status notify_zrange(const lyramilk::data::array& cmd, std::ostream& os)
	{

		lyramilk::data::string sstart = cmd[2].str();
		lyramilk::data::string sstop = cmd[3].str();
		if(sstart.find_first_not_of("0123456789") != lyramilk::data::string::npos || sstop.find_first_not_of("0123456789") != lyramilk::data::string::npos){
			os << "-ERR value is not an integer or out of range\r\n";
			return rs_ok;
		}

		lyramilk::data::int64 start = cmd[2].conv(0ll);
		lyramilk::data::int64 stop = cmd[3].conv(0ll);
		bool withscores = false;
		if(cmd.size() > 4 && lyramilk::data::lower_case(cmd[4].str()) == "withscores"){
			withscores = true;
		}
		lyramilk::data::strings results;
		if(!store->zrange(cmd[1].str(),start,stop,withscores,&results)){
			os << "$-1\r\n";
			return rs_ok;
		}

		{
			os << "*" << results.size() << "\r\n";
			for(lyramilk::data::strings::iterator it = results.begin();it!=results.end();++it){
				os << "$" << it->size() << "\r\n";
				os << *it << "\r\n";
			}
		}
		return rs_ok;
	}

	result_status notify_set(const lyramilk::data::array& cmd, std::ostream& os)
	{
		store->post_command(masterid,"",0,(lyramilk::data::array&)cmd,nullptr,false);
		os << ":1\r\n";
		return rs_ok;
	}

	result_status notify_get(const lyramilk::data::array& cmd, std::ostream& os)
	{
		lyramilk::data::string key = cmd[1].str();

		lyramilk::data::string value;
		if(!store->get(key,&value)){
			os << "$-1\r\n";
		}else{
			os << "$" << value.size() << "\r\n";
			os << value << "\r\n";
		}
		return rs_ok;
	}

	result_status notify_hset(const lyramilk::data::array& cmd, std::ostream& os)
	{
		store->post_command(masterid,"",0,(lyramilk::data::array&)cmd,nullptr,false);
		os << "+OK\r\n";
		return rs_ok;
	}


	result_status notify_hmset(const lyramilk::data::array& cmd, std::ostream& os)
	{
		store->post_command(masterid,"",0,(lyramilk::data::array&)cmd,nullptr,false);
		os << "+OK\r\n";
		return rs_ok;
	}

	result_status notify_hdel(const lyramilk::data::array& cmd, std::ostream& os)
	{
		store->post_command(masterid,"",0,(lyramilk::data::array&)cmd,nullptr,false);
		os << ":1\r\n";
		return rs_ok;
	}

	result_status notify_hget(const lyramilk::data::array& cmd, std::ostream& os)
	{
		lyramilk::data::string key = cmd[1].str();
		lyramilk::data::string field = cmd[2].str();

		lyramilk::data::string value;
		if(!store->hget(key,field,&value)){
			os << "$-1\r\n";
		}else{
			os << "$" << value.size() << "\r\n";
			os << value << "\r\n";
		}
		return rs_ok;
	}

	result_status notify_hgetall(const lyramilk::data::array& cmd, std::ostream& os)
	{
		lyramilk::data::string key = cmd[1].str();

		lyramilk::data::stringdict result = store->hgetall(key);
		if(result.empty()){
			os << "$-1\r\n";
		}else{
			os << "*" << result.size() * 2 << "\r\n";
			for(lyramilk::data::stringdict::iterator it = result.begin();it!=result.end();++it){
				os << "$" << it->first.size() << "\r\n";
				os << it->first << "\r\n";
				os << "$" << it->second.size() << "\r\n";
				os << it->second << "\r\n";
			}
		}
		return rs_ok;
	}

	result_status notify_auth(const lyramilk::data::array& cmd, std::ostream& os)
	{
		if(requirepass != cmd[1].str()){
			os << "-ERR invalid password\r\n";
		}else{
			pass = cmd[1].str();
			os << "+OK\r\n";
		}
		return rs_ok;
	}

	result_status notify_multi(const lyramilk::data::array& cmd, std::ostream& os)
	{
		multi = true;
		os << "+OK\r\n";
		return rs_ok;
	}


	result_status notify_exec(const lyramilk::data::array& cmd, std::ostream& os)
	{
		multi = false;
		os << "+OK\r\n";
		return rs_ok;
	}


	result_status notify_command(const lyramilk::data::array& cmd, std::ostream& os)
	{
		os << "*" << dispatch.size() << "\r\n";
		std::map<lyramilk::data::string,redis_cmd_spec>::const_iterator it = dispatch.begin();
		for(;it!=dispatch.end();++it){
			os << "*6\r\n";

			os << "$" << it->first.size() << "\r\n";
			os << it->first << "\r\n";

			os << ":" << it->second.n << "\r\n";

			lyramilk::data::strings flags;
			#define check_flag(w) if(it->second.f & redis_cmd_spec::w) flags.push_back(#w)
			check_flag(write);
			check_flag(readonly);
			check_flag(denyoom);
			check_flag(admin);
			check_flag(pubsub);
			check_flag(noscript);
			check_flag(random);
			check_flag(sort_for_script);
			check_flag(loading);
			check_flag(stale);
			check_flag(skip_monitor);
			check_flag(asking);
			check_flag(fast);
			//check_flag(noauth);	这个不是redis状态，而是cavedb特有的，故不在command命令中返回。
			#undef check_flag
			os << "*" << flags.size() << "\r\n";
			for(lyramilk::data::strings::iterator fit = flags.begin();fit!=flags.end();++fit){
				os << "+" << *fit << "\r\n";
			}

			os << ":" << it->second.firstkey << "\r\n";
			os << ":" << it->second.lastkey << "\r\n";
			os << ":" << it->second.keystepcount << "\r\n";
		}
		return rs_ok;
	}

	result_status notify_info(const lyramilk::data::array& cmd, std::ostream& os)
	{
		lyramilk::data::strings sinfo;
		sinfo.push_back("# Server");
		sinfo.push_back("  " "cavedb: " CAVEDB_VERSION);
		sinfo.push_back("\r\n");
		{
			os << "*" << sinfo.size() << "\r\n";
			for(lyramilk::data::strings::iterator it = sinfo.begin();it!=sinfo.end();++it){
				os << "$" << it->size() << "\r\n";
				os << *it << "\r\n";
			}
		}
		return rs_ok;
	}

	result_status notify_del(const lyramilk::data::array& cmd, std::ostream& os)
	{
		store->post_command(masterid,"",0,(lyramilk::data::array&)cmd,nullptr,false);
		os << ":1\r\n";
		return rs_ok;
	}


	std::map<lyramilk::data::uint64,lyramilk::data::string> rainbow_table;

	result_status notify_scan(const lyramilk::data::array& cmd, std::ostream& os)
	{
		lyramilk::data::uint64 current_hash = cmd[1].conv(0ull);
		lyramilk::data::string cursor;

		if(current_hash != 0){
			std::map<lyramilk::data::uint64,lyramilk::data::string>::const_iterator it = rainbow_table.find(current_hash);
			if(it!=rainbow_table.end()){
				cursor = it->second;
			}else{
				os << "-ERR invalid cursor\r\n";
				return rs_ok;
			}
		}

		lyramilk::data::strings results;
		lyramilk::data::string next_cursor = store->scan(cursor,50,&results);

		os << "*2\r\n";
		if(next_cursor.empty()){
			os << "$1\r\n0\r\n";
		}else{
			lyramilk::data::uint64 nexthash = lyramilk::cryptology::hash64::fnv(next_cursor.data(),next_cursor.size());
			//lyramilk::data::uint64 nexthash = current_hash + results.size();


			rainbow_table[nexthash] = next_cursor;

			lyramilk::data::string nexthashstr = lyramilk::data::str(nexthash);
			os << "$" << nexthashstr.size() << "\r\n";
			os << nexthashstr << "\r\n";
		}

		{
			os << "*" << results.size() << "\r\n";
			for(lyramilk::data::strings::iterator it = results.begin();it!=results.end();++it){
				os << "$" << it->size() << "\r\n";
				os << *it << "\r\n";
			}
		}
		return rs_ok;
	}

	result_status notify_hscan(const lyramilk::data::array& cmd, std::ostream& os)
	{
		lyramilk::data::uint64 current_hash = cmd[2].conv(0ull);
		lyramilk::data::string cursor;
		if(current_hash != 0){
			std::map<lyramilk::data::uint64,lyramilk::data::string>::const_iterator it = rainbow_table.find(current_hash);
			if(it!=rainbow_table.end()){
				cursor = it->second;
			}else{
				os << "-ERR invalid cursor\r\n";
				return rs_ok;
			}
		}

		lyramilk::data::strings results;
		lyramilk::data::string next_cursor = store->hscan(cmd[1].str(),cursor,50,&results);

		os << "*2\r\n";
		if(next_cursor.empty()){
			os << "$1\r\n0\r\n";
		}else{
			lyramilk::data::uint64 nexthash = lyramilk::cryptology::hash64::fnv(next_cursor.data(),next_cursor.size());
			//lyramilk::data::uint64 nexthash = current_hash + results.size();


			rainbow_table[nexthash] = next_cursor;

			lyramilk::data::string nexthashstr = lyramilk::data::str(nexthash);
			os << "$" << nexthashstr.size() << "\r\n";
			os << nexthashstr << "\r\n";
		}

		{
			os << "*" << results.size() << "\r\n";
			for(lyramilk::data::strings::iterator it = results.begin();it!=results.end();++it){
				os << "$" << it->size() << "\r\n";
				os << *it << "\r\n";
			}
		}
		return rs_ok;
	}

	result_status notify_hlen(const lyramilk::data::array& cmd, std::ostream& os)
	{
		lyramilk::data::uint64 result = store->hlen(cmd[1].str());
		os << ":" << result << "\r\n";
		return rs_ok;
	}

	result_status notify_type(const lyramilk::data::array& cmd, std::ostream& os)
	{
		os << "+" << store->type(cmd[1].str()) << "\r\n";
		return rs_ok;
	}

	result_status notify_monitor(const lyramilk::data::array& cmd, std::ostream& os)
	{
		os << "+OK\r\n";

		store->add_monitor(fd());
		return rs_ok;
	}

	result_status notify_ping(const lyramilk::data::array& cmd, std::ostream& os)
	{
		os << "+PONG\r\n";
		return rs_ok;
	}

	result_status notify_sample(const lyramilk::data::array& cmd, std::ostream& os)
	{
		return rs_ok;
	}

	bool oninit(lyramilk::data::ostream& os)
	{
		return true;
	}

	bool onrequest(const char* cache, int size, lyramilk::data::ostream& os)
	{
		tcpcache.write(cache,size);
		result_status r = parsing(tcpcache,&os);
		if(r == rs_error || r == rs_parse_error){
			return false;
		}
		return true;
	}




	result_status notify_cmd(const lyramilk::data::array& cmd, void* userdata)
	{
		if(cmd.size() < 1) return rs_error;
		std::ostream& os = *(std::ostream*)userdata;

		lyramilk::data::string rcmd = cmd[0].str();
		transform(rcmd.begin(), rcmd.end(), rcmd.begin(), tolower);

		std::map<lyramilk::data::string,redis_cmd_spec>::const_iterator it = dispatch.find(rcmd);
		if(it!=dispatch.end()){
			if((it->second.f&redis_cmd_spec::skip_monitor) == 0){
				store->monitor_lookup(masterid,cmd);
			}

			if(requirepass != pass && it->second.f&redis_cmd_spec::noauth){
				os << "-NOAUTH authentication required.\r\n";
				return rs_ok;
			}

			if(it->second.n >= 0){
				if(cmd.size() != (unsigned long long)it->second.n){
					os << "-ERR wrong number of arguments for '" << cmd[0].str() << "' command\r\n";
					return rs_ok;
				}
			}else{
				unsigned long long nessary = (unsigned long long)(0 - it->second.n);
				if(cmd.size() < nessary){
					os << "-ERR wrong number of arguments for '" << cmd[0].str() << "' command\r\n";
					return rs_ok;
				}
			}
			if(readonly && it->second.f&redis_cmd_spec::readonly){
				os << "+READONLY You can't write against a read only slave.\r\n";
				return rs_ok;
			}

			return (this->*it->second.c)(cmd,os);
		}

		os << "-ERR unknown command '" << rcmd << "'\r\n";
		return rs_ok;
	}

};



class TeapoyDBServer_impl:public lyramilk::netio::aioserver<TeapoyDBServer_session>
{
  public:
	lyramilk::cave::leveldb_standard* store;
	lyramilk::data::string requirepass;
	bool readonly;

	TeapoyDBServer_impl()
	{
		store = nullptr;
	}

	virtual ~TeapoyDBServer_impl()
	{
	
	}
	virtual lyramilk::netio::aiosession* create()
	{
		TeapoyDBServer_session *p = lyramilk::netio::aiosession::__tbuilder<TeapoyDBServer_session>();
		p->store = store;
		p->requirepass = requirepass;
		p->readonly = readonly;
		return p;
	}
};



/*
1605600138.465654 [0 unix:/dev/shm/testredis.sock] "AUTH" "test123abc"
1605600153.989455 [0 unix:/dev/shm/testredis.sock] "AUTH" "test123abc"
1605600153.989573 [0 unix:/dev/shm/testredis.sock] "get" "abc"

*/


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
			lyramilk::log::logf* lf = new lyramilk::log::logf(logfile);
			lyramilk::klog.rebase(lf);
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

	if(isdaemon){
		daemon(0,0);
	}
	signal(SIGPIPE, SIG_IGN);

	// 初始化epoll
	lyramilk::io::aiopoll pool;
	pool.active(10);



	// 初始化leveldb存储
	lyramilk::cave::leveldb_standard* mstore = new lyramilk::cave::leveldb_standard;
	if(!mstore->open_leveldb(gstore["path"].str(),(unsigned int)gstore["cache"].conv(500),true)){
		lyramilk::klog(lyramilk::log::error,"cavedb") << "打开leveldb失败:" << gstore << std::endl;
		return -1;
	}

	//	初始化主库。
	for(lyramilk::data::array::iterator it = gsource.begin();it!=gsource.end();++it){
		if(it->type() == lyramilk::data::var::t_map){
			lyramilk::data::map& m = *it;
			lyramilk::data::string type = m["type"].str();
			if(type == "ssdb"){
				lyramilk::cave::slave_ssdb* datasource = new lyramilk::cave::slave_ssdb;

				lyramilk::data::string ssdb_host = m["host"].str();
				lyramilk::data::int32 ssdb_port = m["port"].conv(-1);
				lyramilk::data::string ssdb_password = m["password"].str();
				lyramilk::data::string masterid = m["masterid"].str();

				lyramilk::data::string replid = "";
				lyramilk::data::uint64 offset = 0;
				mstore->get_sync_info(masterid,&replid,&offset);
				datasource->slaveof(ssdb_host,ssdb_port,ssdb_password,masterid,replid,offset,mstore);
				lyramilk::klog(lyramilk::log::debug,"cavedb") << "同步于ssdb:" << ssdb_host  << ":" << ssdb_port << std::endl;
			}else{
				lyramilk::klog(lyramilk::log::error,"cavedb") << "不支持的source类型：" << type << std::endl;
			}
		}
	}

	TeapoyDBServer_session::static_init_dispatch();

	//	初始化对外服务。
	for(lyramilk::data::array::iterator it = gserver.begin();it!=gserver.end();++it){
		if(it->type() == lyramilk::data::var::t_map){
			lyramilk::data::map& m = *it;
			lyramilk::data::string type = m["type"].str();
			if(type == "network"){
				TeapoyDBServer_impl* p = new TeapoyDBServer_impl;
				p->store = mstore;
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
				TeapoyDBServer_impl* p = new TeapoyDBServer_impl;
				p->store = mstore;
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

	while(true){
		lyramilk::klog(lyramilk::log::debug,"cavedb") << D("wspeed:% 7lu ,rspeed:% 7lu",mstore->wspeed(),mstore->rspeed()) << std::endl;
		sleep(1);
	}
	return 0;
}