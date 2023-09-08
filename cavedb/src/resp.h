#ifndef _cavedb_resp_h_
#define _cavedb_resp_h_

#include <libmilk/var.h>
#include <libmilk/netaio.h>
#include "command.h"
#include <stack>

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	enum resp_result{
		resp_data = 1,
		resp_hidden_data,
		resp_msg_error,
		resp_parse_error,
	};

	resp_result resp23_from_stream(std::istream& is,lyramilk::data::var* ret);
	bool resp23_to_stream(const lyramilk::data::var& ret,lyramilk::data::ostream& os);
	bool resp23_to_stream(const lyramilk::data::array& ret,lyramilk::data::ostream& os);
	bool resp23_to_stream(const lyramilk::data::map& ret,lyramilk::data::ostream& os);

	class resp23_as_session:public lyramilk::netio::aiosession_sync
	{
		enum stream_status{
			s_unknow,
			s_0			= 0x1000,
			s_str_0		= 0x2000,
			s_str_cr,
			s_err_0		= 0x3000,
			s_err_cr,
			s_num_0		= 0x4000,
			s_num_cr,
			s_bulk_0	= 0x5000,
			s_bulk_cr,
			s_bulk_data,
			s_bulk_data_cr,
			s_array_0	= 0x6000,
			s_array_cr,
		}s;
		lyramilk::data::string auth;
		lyramilk::data::var::array data;
		lyramilk::data::string tmpstr;

		lyramilk::data::int64 array_item_count;
		lyramilk::data::int64 bulk_bytes_count;

		virtual bool oninit(lyramilk::data::ostream& os);
		virtual bool onrequest(const char* cache, int size, lyramilk::data::ostream& os);
	public:
		resp23_as_session();
		virtual ~resp23_as_session();

		virtual bool output_redis_result(lyramilk::cave::cmdstatus rs,const lyramilk::data::var& ret,lyramilk::data::ostream& os);
		virtual bool notify_cmd(const lyramilk::data::array& cmd, lyramilk::data::ostream& os) = 0;
	};

}}
#endif
