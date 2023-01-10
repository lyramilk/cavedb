#ifndef _cavedb_cmd_accepter_h_
#define _cavedb_cmd_accepter_h_

#include <libmilk/var.h>
#include "command.h"

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class cmd_accepter;

	template <typename T,cmdstatus (T::*Q)(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen) const>
	static cmdstatus command_method_2_function(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen,void* userptr)
	{
		T* self = (T*)userptr;
		return (self->*(Q))(masterid,replid,offset,args,ret,sen);
	}

	class cmd_accepter
	{
		typedef std::map<lyramilk::data::string,command_sepc,lyramilk::data::case_insensitive_less> cmd_map_type;
		cmd_map_type dispatch;

		cmdstatus on_command(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen) const;
		cmdstatus on_auth(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen) const;
		cmdstatus on_ping(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen) const;
		cmdstatus on_hello(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen) const;

		lyramilk::data::string requirepass;
		int loginseq;
		bool isreadonly;
	  public:
		cmd_accepter();
		virtual ~cmd_accepter();
		void regist(const lyramilk::data::string& cmd,command_callback callback,int argc,int flag,int firstkey_offset,int lastkey_offset,int keystepcount);
		cmdstatus call(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen);
		cmdstatus call(const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen);

		void set_requirepass(const lyramilk::data::string& requirepass);
		void set_readonly(bool isreadonly);
	};
}}

#endif
