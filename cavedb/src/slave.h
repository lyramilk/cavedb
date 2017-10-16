#ifndef _casedb_slave_h_
#define _casedb_slave_h_

#include <libmilk/var.h>
/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class slave
	{
	  public:
		virtual void notify_command(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args) = 0;
		virtual bool notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset) = 0;
		virtual bool notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset) = 0;
	};
}}

#endif
