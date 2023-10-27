#ifndef _cavedb_command_h_
#define _cavedb_command_h_

#include <libmilk/var.h>

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{

	struct cmdsessiondata
	{
		bool allowslowcommand;
		int loginseq;
		lyramilk::data::string pass; 
	};


	struct cmdchanneldata
	{
		lyramilk::data::string requirepass;
		bool isreadonly;
		int loginseq;

		cmdchanneldata();
		~cmdchanneldata();
		void set_requirepass(const lyramilk::data::string& requirepass);
		void set_readonly(bool isreadonly);
	};



	enum cmdstatus{
		cs_ok,
		cs_error,
		cs_data,
		cs_data_not_found,
	};

	typedef cmdstatus (*command_callback)(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen,void* userptr);

	struct command_sepc
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
		const static int slow = 0x4000;// 非redis状态，而是cavedb特有的，表示这个命令与redis不同，它运行非常慢。

		command_callback invoke;
		int argc;
		int flag;
		int firstkey;
		int lastkey;
		int keystepcount;
	};

}}
#endif