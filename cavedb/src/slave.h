#ifndef _casedb_slave_h_
#define _casedb_slave_h_

#include <libmilk/var.h>
/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class slave
	{
	  public:
		enum master_type{
			slave_unknow,
			slave_of_ssdb,
			slave_of_redis,
		};

		virtual bool notify_command(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args) = 0;
		virtual bool notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset) = 0;
		virtual bool notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset) = 0;
	};
}}

#endif
