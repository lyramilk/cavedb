#ifndef _cavedb_slave_h_
#define _cavedb_slave_h_

#include <libmilk/var.h>
/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class slave
	{
		friend class slave_ssdb;
		friend class slave_redis;
		bool is_in_full_sync;
	  public:
		enum master_type{
			slave_unknow,
			slave_of_ssdb,
			slave_of_redis,
		};

		slave()
		{
			//在slave_ssdb和slave_redis中赋值
			is_in_full_sync = true;
		}

		virtual ~slave()
		{
		}
		 //是否正在进行全量同步。
		virtual bool on_full_sync()
		{
			return is_in_full_sync;
		}

		virtual bool dispatch_command(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata) = 0;
		virtual bool notify_psync(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata) = 0;
		virtual bool notify_idle(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata) = 0;
	};
}}

#endif
