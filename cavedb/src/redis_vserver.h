#ifndef _cavedb_redis_vserver_h_
#define _cavedb_redis_vserver_h_

#include <libmilk/thread.h>
#include <libmilk/netio.h>

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class redis_vserver
	{
		lyramilk::data::string auth;
		lyramilk::data::array data;
		lyramilk::data::string tmpstr;
		enum redis_stream_status{
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
			s_bulk_data_0,
			s_bulk_data_cr,
			s_array_0	= 0x6000,
			s_array_cr,
		}s;
		lyramilk::data::int64 array_item_count;
		lyramilk::data::int64 bulk_bytes_count;
	  public:
		redis_vserver()
		{
			array_item_count = 0;
			bulk_bytes_count = 0;
			s = s_0;
		}

	  	virtual ~redis_vserver()
		{
		}

	  protected:
		bool parsing(char c,void* userdata)
		{
			switch(s){
			case s_0:
				if(c == '+') s = s_str_0;
				else if(c == '-') s = s_err_0;
				else if(c == ':') s = s_num_0;
				else if(c == '$') s = s_bulk_0;
				else if(c == '*') s = s_array_0;
				else return false;
				break;
			case s_str_0:
			case s_err_0:
			case s_num_0:
			case s_array_0:
			case s_bulk_0:
				if(c =='\r') s = (redis_stream_status)(s + 1);
				tmpstr.push_back(c);
				break;
			case s_str_cr:
			case s_err_cr:
			case s_num_cr:
			case s_bulk_cr:
			case s_array_cr:
				if(c =='\n'){
					if(s == s_str_cr){
						data.push_back(tmpstr);
						tmpstr.clear();
						if(array_item_count > 0){
							--array_item_count;
						}
						s = s_0;
					}else if(s == s_num_cr){
						char* p;
						lyramilk::data::int64 i = strtoll(tmpstr.c_str(),&p,10);
						data.push_back(i);
						tmpstr.clear();
						if(array_item_count > 0){
							--array_item_count;
						}
						s = s_0;
					}else if(s == s_array_cr){
						char* p;
						array_item_count = strtoll(tmpstr.c_str(),&p,10);
						tmpstr.clear();
						s = s_0;
					}else if(s == s_bulk_cr){
						char* p;
						bulk_bytes_count = strtoll(tmpstr.c_str(),&p,10);
						tmpstr.clear();
						s = s_bulk_data;
					}
				}else return false;
				break;
			case s_bulk_data:
				if(bulk_bytes_count == 0){
					if(c == '\r') s = s_bulk_data_cr;
					else return false;
				}else{
					--bulk_bytes_count;
					tmpstr.push_back(c);
				}
				break;
			case s_bulk_data_cr:
				if(c == '\n'){
					data.push_back(tmpstr);
					tmpstr.clear();
					if(array_item_count > 0){
						--array_item_count;
					}
					s = s_0;
				}else return false;
				break;
			default:
				return false;
			}
			if(s == s_0 && array_item_count == 0){
				bool r = notify_cmd(data,userdata);
				data.clear();
				return r;
			}
			return true;
		}

		virtual bool notify_cmd(const lyramilk::data::array& cmd, void* userdata) = 0;

	};
}}

#endif
