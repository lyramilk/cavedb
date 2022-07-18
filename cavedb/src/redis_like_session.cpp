#include "redis_like_session.h"
#include "config.h"
#include <algorithm>

namespace lyramilk{ namespace cave
{
	redislike_dispatch_type redislike_session::dispatch;







	void redislike_session::regist_command(redislike_dispatch_type *dispatch,const lyramilk::data::string& cmd,redis_cmd_callback callback,int argcount,int flag,int firstkey_offset,int lastkey_offset,int keystepcount)
	{
		dispatch->operator[](cmd).c = callback;
		dispatch->operator[](cmd).f = flag;
		dispatch->operator[](cmd).firstkey = firstkey_offset;
		dispatch->operator[](cmd).lastkey = lastkey_offset;
		dispatch->operator[](cmd).keystepcount = keystepcount;
		dispatch->operator[](cmd).n = argcount;
	}

	void redislike_session::static_init_dispatch()
	{
		regist_command(&redislike_session::dispatch,"cavedb_sync",&redislike_session::notify_cavedb_sync,1,redis_cmd_spec::readonly|redis_cmd_spec::admin|redis_cmd_spec::noscript,0,0,0);
		regist_command(&redislike_session::dispatch,"command",&redislike_session::notify_command,1,redis_cmd_spec::readonly|redis_cmd_spec::skip_monitor|redis_cmd_spec::fast|redis_cmd_spec::noscript|redis_cmd_spec::noauth,0,0,0);

		regist_command(&redislike_session::dispatch,"auth",&redislike_session::notify_auth,2,redis_cmd_spec::readonly|redis_cmd_spec::loading|redis_cmd_spec::noauth|redis_cmd_spec::fast|redis_cmd_spec::noscript,0,0,0);
		regist_command(&redislike_session::dispatch,"del",&redislike_session::notify_del,2,redis_cmd_spec::write|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);
		regist_command(&redislike_session::dispatch,"ping",&redislike_session::notify_ping,1,redis_cmd_spec::readonly|redis_cmd_spec::skip_monitor|redis_cmd_spec::fast|redis_cmd_spec::noscript,0,0,0);
		regist_command(&redislike_session::dispatch,"monitor",&redislike_session::notify_monitor,1,redis_cmd_spec::readonly|redis_cmd_spec::skip_monitor|redis_cmd_spec::admin|redis_cmd_spec::noscript,0,0,0);

		regist_command(&redislike_session::dispatch,"hgetall",&redislike_session::notify_hgetall,2,redis_cmd_spec::readonly|redis_cmd_spec::noscript,1,1,1);
		regist_command(&redislike_session::dispatch,"hget",&redislike_session::notify_hget,3,redis_cmd_spec::readonly|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);
		regist_command(&redislike_session::dispatch,"hmget",&redislike_session::notify_hmget,-3,redis_cmd_spec::readonly|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);
		regist_command(&redislike_session::dispatch,"hexist",&redislike_session::notify_hexist,3,redis_cmd_spec::readonly|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);
		regist_command(&redislike_session::dispatch,"hset",&redislike_session::notify_hset,4,redis_cmd_spec::write|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);
		regist_command(&redislike_session::dispatch,"hmset",&redislike_session::notify_hmset,-4,redis_cmd_spec::write|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);
		regist_command(&redislike_session::dispatch,"hdel",&redislike_session::notify_hdel,-3,redis_cmd_spec::write|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);

		regist_command(&redislike_session::dispatch,"sadd",&redislike_session::notify_sadd,3,redis_cmd_spec::write|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);
		regist_command(&redislike_session::dispatch,"srem",&redislike_session::notify_srem,3,redis_cmd_spec::write|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);

		regist_command(&redislike_session::dispatch,"zadd",&redislike_session::notify_zadd,4,redis_cmd_spec::write|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);
		regist_command(&redislike_session::dispatch,"zrem",&redislike_session::notify_zrem,3,redis_cmd_spec::write|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);
		regist_command(&redislike_session::dispatch,"set",&redislike_session::notify_set,3,redis_cmd_spec::write|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);

		regist_command(&redislike_session::dispatch,"config",&redislike_session::notify_config,-2,redis_cmd_spec::readonly|redis_cmd_spec::fast|redis_cmd_spec::skip_monitor|redis_cmd_spec::admin|redis_cmd_spec::noscript,0,0,0);
		regist_command(&redislike_session::dispatch,"client",&redislike_session::notify_client,-2,redis_cmd_spec::readonly|redis_cmd_spec::fast|redis_cmd_spec::skip_monitor|redis_cmd_spec::admin|redis_cmd_spec::noscript,0,0,0);

		/*
		regist_command(&redislike_session::dispatch,"multi",&redislike_session::notify_multi,1,redis_cmd_spec::readonly|redis_cmd_spec::fast|redis_cmd_spec::noscript,0,0,0);
		regist_command(&redislike_session::dispatch,"exec",&redislike_session::notify_exec,1,redis_cmd_spec::readonly|redis_cmd_spec::fast|redis_cmd_spec::noscript,0,0,0);
		*/
	}

	redislike_session::redislike_session()
	{
		readonly = false;
		allowslowcommand = false;
		store = nullptr;
		reader = nullptr;
		session_with_monitor = false;
	}

	redislike_session::~redislike_session()
	{
		if(session_with_monitor){
			store->remove_monitor(fd());
		}
	}

	void redislike_session::init_cavedb(const lyramilk::data::string& masterid,const lyramilk::data::string& requirepass,lyramilk::cave::store* store,lyramilk::cave::store_reader* reader,bool readonly)
	{
		this->store = store;
		this->reader = reader;
		this->requirepass = requirepass;
		this->readonly = readonly;
		this->masterid = masterid;
	}

	lyramilk::cave::redis_session::result_status redislike_session::notify_cmd(const lyramilk::data::array& cmd, void* userdata)
	{
		if(cmd.size() < 1) return rs_error;
		std::ostream& os = *(std::ostream*)userdata;

		lyramilk::data::string rcmd = cmd[0].str();
		transform(rcmd.begin(), rcmd.end(), rcmd.begin(), tolower);

		const redis_cmd_spec* spec = nullptr;

		redislike_dispatch_type::const_iterator it = dispatch.find(rcmd);
		if(it == dispatch.end()){
			for(std::vector<redislike_dispatch_type*>::iterator ait = dispatch_child.begin();ait != dispatch_child.end(); ++ait){
				redislike_dispatch_type* p = *ait;
				if(p != nullptr){
					it = p->find(rcmd);
					if(it != p->end()){
						spec = &it->second;
						break;
					}
				}
			}
		}else{
			spec = &it->second;
		}


		if(spec == nullptr){
			os << "-ERR unknown command '" << rcmd << "'\r\n";
			return rs_ok;
		}

		if(requirepass != pass && !(it->second.f&redis_cmd_spec::noauth)){
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

		bool is_readonly_cmd = (it->second.f&redis_cmd_spec::readonly) != 0;

		if(readonly && !is_readonly_cmd){
			os << "+READONLY You can't write against a read only replica.\r\n";
			return rs_ok;
		}

		if((!allowslowcommand) && it->second.f&redis_cmd_spec::slow){
			os << "-ERR This command maybe very slow. Please use command 'client allowslowcommand 1' to enable slow command if you understand that.\r\n";
			return rs_ok;
		}

		if((it->second.f&redis_cmd_spec::skip_monitor) == 0){
			store->monitor_lookup(masterid,cmd);
		}
		return (this->*it->second.c)(cmd,os);
	}


	lyramilk::cave::redis_session::result_status redislike_session::notify_auth(const lyramilk::data::array& cmd, std::ostream& os)
	{
		if(requirepass != cmd[1].str()){
			os << "-ERR invalid password\r\n";
		}else{
			pass = cmd[1].str();
			os << "+OK\r\n";
		}
		return rs_ok;
	}

	lyramilk::cave::redis_session::result_status redislike_session::notify_cavedb_sync(const lyramilk::data::array& cmd, std::ostream& os)
	{
		os << "-ERR Command will implement future.\r\n";
		return rs_ok;
	}

	lyramilk::cave::redis_session::result_status redislike_session::notify_command(const lyramilk::data::array& cmd, std::ostream& os)
	{
		os << "*" << dispatch.size() << "\r\n";
		redislike_dispatch_type::const_iterator it = dispatch.begin();
		for(;it!=dispatch.end();++it){
			os << "*6\r\n";

			os << "$" << it->first.size() << "\r\n";
			os << it->first << "\r\n";

			os << ":" << it->second.n << "\r\n";

			lyramilk::data::strings flags;
			flags.reserve(4);
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
			//check_flag(noauth);	//	这个不是redis状态，而是cavedb特有的，故不在command命令中返回。
			check_flag(slow);	//	非redis状态，而是cavedb特有的，表示这个命令与redis不同，它运行非常慢。
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

	lyramilk::cave::redis_session::result_status redislike_session::notify_del(const lyramilk::data::array& cmd, std::ostream& os)
	{
		store->post_command(masterid,"",0,(lyramilk::data::array&)cmd,nullptr,false);
		os << ":1\r\n";
		return rs_ok;
	}

	lyramilk::cave::redis_session::result_status redislike_session::notify_ping(const lyramilk::data::array& cmd, std::ostream& os)
	{
		os << "+PONG\r\n";
		return rs_ok;
	}

	lyramilk::cave::redis_session::result_status redislike_session::notify_monitor(const lyramilk::data::array& cmd, std::ostream& os)
	{
		os << "+OK\r\n";
		if(store->add_monitor(fd())){
			session_with_monitor = true;
		}
		return rs_ok;
	}



	lyramilk::cave::redis_session::result_status redislike_session::notify_sample(const lyramilk::data::array& cmd, std::ostream& os)
	{
		os << "-ERR Sample!!!.\r\n";
		return rs_ok;
	}

	//	hashmap
	lyramilk::cave::redis_session::result_status redislike_session::notify_hgetall(const lyramilk::data::array& cmd, std::ostream& os)
	{
		lyramilk::data::string key = cmd[1].str();

		lyramilk::data::stringdict result = reader->hgetall(key);
		if(result.empty()){
			os << "*0\r\n\r\n";
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

	lyramilk::cave::redis_session::result_status redislike_session::notify_hget(const lyramilk::data::array& cmd, std::ostream& os)
	{
		lyramilk::data::string key = cmd[1].str();
		lyramilk::data::string field = cmd[2].str();

		lyramilk::data::string value;
		if(!reader->hget(key,field,&value)){
			os << "$-1\r\n";
		}else{
			os << "$" << value.size() << "\r\n";
			os << value << "\r\n";
		}
		return rs_ok;
	}

	lyramilk::cave::redis_session::result_status redislike_session::notify_hmget(const lyramilk::data::array& cmd, std::ostream& os)
	{
		lyramilk::data::string key = cmd[1].str();

		os << "*" << cmd.size() - 2 << "\r\n";

		for(unsigned int i=2;i<cmd.size();++i){
			lyramilk::data::string value;
			lyramilk::data::string field = cmd[i].str();
			if(!reader->hget(key,field,&value)){
				os << "$-1\r\n";
			}else{
				os << "$" << value.size() << "\r\n";
				os << value << "\r\n";
			}
		}
		return rs_ok;
	}

	lyramilk::cave::redis_session::result_status redislike_session::notify_hexist(const lyramilk::data::array& cmd, std::ostream& os)
	{
		lyramilk::data::string key = cmd[1].str();
		lyramilk::data::string field = cmd[2].str();

		lyramilk::data::string value;
		if(reader->hget(key,field,&value)){
			//os << "+OK\r\n";
			os << ":1\r\n";
		}else{
			os << "$-1\r\n";
		}
		return rs_ok;
	}

	lyramilk::cave::redis_session::result_status redislike_session::notify_hset(const lyramilk::data::array& cmd, std::ostream& os)
	{
		store->post_command(masterid,"",0,(lyramilk::data::array&)cmd,nullptr,false);
		os << "+OK\r\n";
		return rs_ok;
	}


	lyramilk::cave::redis_session::result_status redislike_session::notify_hmset(const lyramilk::data::array& cmd, std::ostream& os)
	{
		store->post_command(masterid,"",0,(lyramilk::data::array&)cmd,nullptr,false);
		os << "+OK\r\n";
		return rs_ok;
	}

	lyramilk::cave::redis_session::result_status redislike_session::notify_hdel(const lyramilk::data::array& cmd, std::ostream& os)
	{
		store->post_command(masterid,"",0,(lyramilk::data::array&)cmd,nullptr,false);
		os << ":1\r\n";
		return rs_ok;
	}

	//	set
	lyramilk::cave::redis_session::result_status redislike_session::notify_sadd(const lyramilk::data::array& cmd, std::ostream& os)
	{
		store->post_command(masterid,"",0,(lyramilk::data::array&)cmd,nullptr,false);
		os << ":1\r\n";
		return rs_ok;
	}

	lyramilk::cave::redis_session::result_status redislike_session::notify_srem(const lyramilk::data::array& cmd, std::ostream& os)
	{
		store->post_command(masterid,"",0,(lyramilk::data::array&)cmd,nullptr,false);
		os << ":1\r\n";
		return rs_ok;
	}


	//	zset
	lyramilk::cave::redis_session::result_status redislike_session::notify_zadd(const lyramilk::data::array& cmd, std::ostream& os)
	{
		store->post_command(masterid,"",0,(lyramilk::data::array&)cmd,nullptr,false);
		os << ":1\r\n";
		return rs_ok;
	}

	lyramilk::cave::redis_session::result_status redislike_session::notify_zrem(const lyramilk::data::array& cmd, std::ostream& os)
	{
		store->post_command(masterid,"",0,(lyramilk::data::array&)cmd,nullptr,false);
		os << ":1\r\n";
		return rs_ok;
	}


	lyramilk::cave::redis_session::result_status redislike_session::notify_set(const lyramilk::data::array& cmd, std::ostream& os)
	{
		store->post_command(masterid,"",0,(lyramilk::data::array&)cmd,nullptr,false);
		os << ":1\r\n";
		return rs_ok;
	}

	lyramilk::cave::redis_session::result_status redislike_session::notify_config(const lyramilk::data::array& cmd, std::ostream& os)
	{
		os << "-ERR Command will implement future\r\n";
		return rs_ok;
	}

	lyramilk::cave::redis_session::result_status redislike_session::notify_client(const lyramilk::data::array& cmd, std::ostream& os)
	{
		lyramilk::data::string subcmd = cmd[1].str();

		if(subcmd == "allowslowcommand"){
			int c = cmd[2].conv(0);
			if(1 == c){
				allowslowcommand = true;
			}else if( 0 == c){
				allowslowcommand = false;
			}
			os << "+OK\r\n";
			return rs_ok;
		}

		os << "-ERR Unknown subcommand or wrong number of arguments for '" << subcmd << "'\r\n";
		return rs_ok;
	}

	bool redislike_session::oninit(lyramilk::data::ostream& os)
	{
		return true;
	}

	bool redislike_session::onrequest(const char* cache, int size, lyramilk::data::ostream& os)
	{

		int bytesusedtotal = 0;
		while(bytesusedtotal < size){
			int bytesused = 0;
			lyramilk::cave::redis_session::result_status r = parsing(cache + bytesusedtotal,size - bytesusedtotal,&bytesused,&os);
			bytesusedtotal += bytesused;
			if(r == rs_error || r == rs_parse_error){
				return false;
			}
		}
		return true;
	}


}}




/*

*/