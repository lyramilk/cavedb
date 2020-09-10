#ifndef _cavedb_slave_h_redis_session_h_
#define _cavedb_slave_h_redis_session_h_

#include <libmilk/netaio.h>
#include <libmilk/netproxy.h>
#include <stdlib.h>
#include <libmilk/testing.h>


namespace lyramilk{ namespace cave
{

	class redis_session
	{
		lyramilk::data::string auth;
		lyramilk::data::var::array data;
		lyramilk::data::string tmpstr;
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
			s_ssdb_s0	= 0x7000,
			s_ssdb_str_0,
			s_ssdb_str_cr,
			s_ssdb_str_data,
			s_ssdb_str_data_cr,
			s_http_s0	= 0x8000,
			s_http_cr,
			s_http_crlf,
			s_http_crlfcr,
		}s;
		lyramilk::data::int64 array_item_count;
		lyramilk::data::int64 bulk_bytes_count;
	  public:
		enum result_status{
			rs_continue,
			rs_ok,
			rs_parse_error,
			rs_error,
		};

		enum session_type{
			st_unknow,
			st_redis,
			st_ssdb,
			st_http,
		};
	  public:
		redis_session();


		static bool parse(lyramilk::data::istream& is,lyramilk::data::var& v,bool* onerr);
		static lyramilk::data::var exec_redis(lyramilk::netio::client& c,const lyramilk::data::array& cmd,bool* onerr);
		static lyramilk::data::strings exec_ssdb(lyramilk::netio::client& c,const lyramilk::data::array& cmd,bool* onerr);
	  protected:
	  	virtual ~redis_session();
		session_type stype;
		result_status parsing(char c,void* userdata);
		result_status parsing(const char* cache, int size,int* bytesused,void* userdata);
		result_status parsing(lyramilk::data::istream& is,void* userdata);
		virtual result_status notify_cmd(const lyramilk::data::var::array& cmd, void* userdata) = 0;
		virtual result_status notify_httpget(void* userdata);
	};
}}
#endif