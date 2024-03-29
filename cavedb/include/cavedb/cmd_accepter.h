#ifndef _cavedb_cmd_accepter_h_
#define _cavedb_cmd_accepter_h_

#include <libmilk/var.h>
#include "command.h"
#include "binlog_store.h"

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class cmd_accepter;

	template <typename T,cmdstatus (T::*Q)(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const>
	static cmdstatus command_method_2_function(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen,void* userptr)
	{
		T* self = (T*)userptr;
		return (self->*(Q))(masterid,replid,offset,args,ret,chd,sen);
	}

	class cmd_accepter
	{
		typedef std::map<lyramilk::data::string,command_sepc,lyramilk::data::case_insensitive_less> cmd_map_type;
		cmd_map_type dispatch;

		cmdstatus on_command(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_auth(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_ping(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_hello(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;

		cmdstatus on_sync_start(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_sync_idle(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_sync_continue(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_sync_overflow(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
	  protected:
		bool is_in_full_sync;
		bool is_master;
		binlog* blog;
	  public:
		cmd_accepter();
		virtual ~cmd_accepter();
		void regist(const lyramilk::data::string& cmd,command_callback callback,int argc,int flag,int firstkey_offset,int lastkey_offset,int keystepcount);
		cmdstatus call(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen);
		cmdstatus call(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,cmdchanneldata* chd,cmdsessiondata* sen);

		virtual bool check_command(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen,const command_sepc& cmdspec);
		virtual void after_command(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen,const command_sepc& cmdspec,cmdstatus retcs);

		void set_full_sync_completed(bool iscompleted);

		virtual void set_master(bool is_master);
		virtual void set_binlog(binlog* blog);
		virtual bool save_sync_info(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset) const = 0;
		virtual bool get_sync_info(const lyramilk::data::string& masterid,lyramilk::data::string* replid,lyramilk::data::uint64* offset) const = 0;
	};
}}

#endif
