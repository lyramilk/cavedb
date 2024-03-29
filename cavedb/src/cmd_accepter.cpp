#include "cmd_accepter.h"
#include <libmilk/log.h>
#include <libmilk/dict.h>
/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{



	cmdstatus cmd_accepter::on_command(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		ret->type(lyramilk::data::var::t_array);

		lyramilk::data::array& ar = *ret;
		for(cmd_map_type::const_iterator it = dispatch.begin();it!=dispatch.end();++it){
			if(it->second.flag & command_sepc::hidden) continue;
			lyramilk::data::var v;
			v.type(lyramilk::data::var::t_array);
			lyramilk::data::array& cmdinfos = v;


			cmdinfos.push_back(it->first);
			cmdinfos.push_back(it->second.argc);
			lyramilk::data::array flags;
			flags.reserve(4);
			#define check_flag(w) if(it->second.flag & command_sepc::w) flags.push_back(#w)
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
			cmdinfos.push_back(flags);

			cmdinfos.push_back(it->second.firstkey);
			cmdinfos.push_back(it->second.lastkey);
			cmdinfos.push_back(it->second.keystepcount);

			ar.push_back(v);
		}
		return cs_data;
	}
	cmdstatus cmd_accepter::on_auth(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		if(chd->requirepass != args[1].str()){
			*ret = "ERR invalid password";
			return cs_error;
		}else{
			sen->pass = args[1].str();
			*ret = "OK";
			return cs_ok;
		}
	}

	cmdstatus cmd_accepter::on_ping(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		*ret = "PONG";
		return cs_ok;
	}

	cmdstatus cmd_accepter::on_hello(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		*ret = "OK";
		return cs_ok;
	}

	cmdstatus cmd_accepter::on_sync_start(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		//*ret = "OK";
		return cs_ok;
	}

	cmdstatus cmd_accepter::on_sync_idle(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		//*ret = "OK";
		return cs_ok;
	}

	cmdstatus cmd_accepter::on_sync_continue(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		//*ret = "OK";
		return cs_ok;
	}

	cmdstatus cmd_accepter::on_sync_overflow(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const
	{
		lyramilk::klog(lyramilk::log::error,"lyramilk.cave.cmd_accepter.on_sync_overflow") << D("主从同步序号溢出binlog范围") << std::endl;
		//*ret = "OK";
		return cs_error;
	}

	cmd_accepter::cmd_accepter()
	{
		regist("command",command_method_2_function<cmd_accepter,&cmd_accepter::on_command>,1,command_sepc::readonly|command_sepc::skip_monitor|command_sepc::fast|command_sepc::noscript|command_sepc::noauth,0,0,0);
		regist("auth",command_method_2_function<cmd_accepter,&cmd_accepter::on_auth>,2,command_sepc::readonly|command_sepc::loading|command_sepc::noauth|command_sepc::fast|command_sepc::noscript,0,0,0);
		regist("ping",command_method_2_function<cmd_accepter,&cmd_accepter::on_ping>,1,command_sepc::readonly|command_sepc::skip_monitor|command_sepc::fast|command_sepc::noscript,0,0,0);
		//regist("hello",command_method_2_function<cmd_accepter,&cmd_accepter::on_hello>,-2,command_sepc::readonly|command_sepc::stale|command_sepc::skip_monitor|command_sepc::fast|command_sepc::loading|command_sepc::noscript|command_sepc::noauth,0,0,0);
		regist("sync_start",command_method_2_function<cmd_accepter,&cmd_accepter::on_sync_start>,1,command_sepc::readonly|command_sepc::skip_monitor|command_sepc::fast|command_sepc::noscript|command_sepc::hidden,0,0,0);
		regist("sync_idle",command_method_2_function<cmd_accepter,&cmd_accepter::on_sync_idle>,1,command_sepc::readonly|command_sepc::skip_monitor|command_sepc::fast|command_sepc::noscript|command_sepc::hidden,0,0,0);
		regist("sync_continue",command_method_2_function<cmd_accepter,&cmd_accepter::on_sync_continue>,1,command_sepc::readonly|command_sepc::skip_monitor|command_sepc::fast|command_sepc::noscript|command_sepc::hidden,0,0,0);
		regist("sync_overflow",command_method_2_function<cmd_accepter,&cmd_accepter::on_sync_overflow>,1,command_sepc::write|command_sepc::skip_monitor|command_sepc::fast|command_sepc::noscript|command_sepc::hidden,0,0,0);
		is_master = false;
	}
	cmd_accepter::~cmd_accepter()
	{
		
	}

	void cmd_accepter::regist(const lyramilk::data::string& cmd,command_callback callback,int argc,int flag,int firstkey_offset,int lastkey_offset,int keystepcount)
	{
		command_sepc& spec = dispatch[cmd];
		spec.invoke = callback;
		spec.flag = flag;
		spec.firstkey = firstkey_offset;
		spec.lastkey = lastkey_offset;
		spec.keystepcount = keystepcount;
		spec.argc = argc;
	}

	cmdstatus cmd_accepter::call(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen)
	{
		lyramilk::data::string cmd = args[0].str();
		
		cmd_map_type::iterator it = dispatch.find(cmd);
		if(it!=dispatch.end()){

			// 检查命令是否需要登录
			if(sen->loginseq != chd->loginseq){
				if(sen->pass == chd->requirepass){
					sen->loginseq = chd->loginseq;
				}
			}
			if(sen->loginseq != chd->loginseq && !(it->second.flag&command_sepc::noauth)){
				*ret = "NOAUTH authentication required.";
				return cs_error;
			}

			// 检查命令是否只读
			bool is_readonly_cmd = (it->second.flag&command_sepc::readonly) != 0;
			if(chd->isreadonly && !is_readonly_cmd){
				*ret = "READONLY You can't write against a read only replica.";
				return cs_error;
			}


			// 检查是否慢速指令
			if((!sen->allowslowcommand) && it->second.flag&command_sepc::slow){
				*ret = "-ERR This command maybe very slow. Please use command 'client allowslowcommand 1' to enable slow command if you understand that.\r\n";
				return cs_error;
			}

			// 检查命令参数数量
			if(it->second.argc >= 0){
				if(args.size() != (unsigned long long)it->second.argc){
					*ret = "ERR wrong number of arguments for '" + cmd + "' command";
					return cs_error;
				}
			}else{
				unsigned long long nessary = (unsigned long long)(0 - it->second.argc);
				if(args.size() < nessary){
					*ret = "ERR wrong number of arguments for '" + cmd + "' command";
					return cs_error;
				}
			}

			if(!check_command(masterid,replid,offset,args,ret,chd,sen,it->second)){
				*ret = "ERR '" + cmd + "' execute fail";
				return cs_error;
			}

			cmdstatus cs = it->second.invoke(masterid,replid,offset,args,ret,chd,sen,this);
			if(cs == cmdstatus::cs_ok || cs == cmdstatus::cs_data){
				after_command(masterid,replid,offset,args,ret,chd,sen,it->second,cs);
			}
			return cs;
		}

		*ret = "ERR unknown command '" + cmd + "'";
		return cs_error;
	}


	cmdstatus cmd_accepter::call(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,cmdchanneldata* chd,cmdsessiondata* sen)
	{
		lyramilk::data::var ret;
		return call(masterid,replid,offset,args,&ret,chd,sen);
	}

	bool cmd_accepter::check_command(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen,const command_sepc& cmdspec)
	{
		return true;
	}

	void cmd_accepter::after_command(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen,const command_sepc& cmdspec,cmdstatus retcs)
	{
	}


	void cmd_accepter::set_master(bool is_master)
	{
		if(this->blog){
			this->blog->set_master(is_master);
		}
		this->is_master = is_master;
	}

	void cmd_accepter::set_binlog(binlog* blog)
	{
		this->blog = blog;
		if(this->blog){
			this->blog->set_master(is_master);
		}
	}

	void cmd_accepter::set_full_sync_completed(bool iscompleted)
	{
		this->is_in_full_sync = iscompleted;
	}

}}





