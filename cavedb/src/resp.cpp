#include "resp.h"

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{

	// resp3_as_session

	resp3_as_session::resp3_as_session()
	{
	}

	resp3_as_session::~resp3_as_session()
	{
		
	}


	bool resp3_as_session::output_redis_data(const lyramilk::data::var& ret,lyramilk::data::ostream& os)
	{
		TODO();
	}

	bool resp3_as_session::output_redis_result(lyramilk::cave::cmdstatus rs,const lyramilk::data::var& ret,lyramilk::data::ostream& os)
	{
		TODO();
	}


	bool resp3_as_session::onrequest(const char* cache, int size, lyramilk::data::ostream& os)
	{
		TODO();
	}










	// resp23_as_session

	resp23_as_session::resp23_as_session()
	{
		s = s_0;
		resp3_adapter = nullptr;
	}

	resp23_as_session::~resp23_as_session()
	{
		
	}

	bool resp23_as_session::oninit(lyramilk::data::ostream& os)
	{
		return true;
	}



	bool resp23_as_session::output_redis_data(const lyramilk::data::var& ret,lyramilk::data::ostream& os)
	{
		if(ret.type() == lyramilk::data::var::t_str){
			lyramilk::data::string str = ret.str();
			os << "$" << str.size() << "\r\n";
			os << str << "\r\n";
			return true;
		}else if(ret.type() == lyramilk::data::var::t_wstr){
			lyramilk::data::string str = ret.str();
			os << "$" << str.size() << "\r\n";
			os << str << "\r\n";
			return true;
		}else if(ret.type() == lyramilk::data::var::t_int){
			os << ":" << (long long)ret << "\r\n";
			return true;
		}else if(ret.type() == lyramilk::data::var::t_array){
			const lyramilk::data::array& ar = ret;
			os << "*" << ar.size() << "\r\n";
			for(lyramilk::data::array::const_iterator it = ar.begin();it !=ar.end();++it){
				if(!output_redis_data(*it,os)) return false;
			}
			return true;
		}else if(ret.type() == lyramilk::data::var::t_map){
			const lyramilk::data::map& ar = ret;
			os << "*" << (ar.size() << 1) << "\r\n";
			for(lyramilk::data::map::const_iterator it = ar.begin();it !=ar.end();++it){
				if(!output_redis_data(it->first,os)) return false;
				if(!output_redis_data(it->second,os)) return false;
			}
			return true;
		}else if(ret.type() == lyramilk::data::var::t_invalid){
			os << "$-1\r\n";
			return true;
		}else{
			return false;
		}
		
	}

	bool resp23_as_session::output_redis_result(lyramilk::cave::cmdstatus rs,const lyramilk::data::var& ret,lyramilk::data::ostream& os)
	{
		if(rs == lyramilk::cave::cs_data){
			return output_redis_data(ret,os);
		}else if(rs == lyramilk::cave::cs_data_not_found){
			os << "$-1\r\n";
			return true;
		}else if(rs == lyramilk::cave::cs_error){
			if(ret.type_like(lyramilk::data::var::t_str)){
				os << "-" <<  ret.str() << "\r\n";
			}else{
				os << "-ERR unknown error\r\n";
			}
			return true;
		}else if(rs == lyramilk::cave::cs_ok){
			if(ret.type_like(lyramilk::data::var::t_str)){
				os << "+" <<  ret.str() << "\r\n";
			}else{
				os << "+OK\r\n";
			}
			return true;
		}

		return false;
	}

	bool resp23_as_session::onrequest(const char* cache, int size, lyramilk::data::ostream& os)
	{
		//if(resp3_adapter) return resp3_adapter->onrequest(cache,size,os);

		for(const char *p = cache;p<cache+size;++p){
			char c = *p;
			switch(s){
			case s_0:
				if(c == '+') s = s_str_0;
				else if(c == '-') s = s_err_0;
				else if(c == ':') s = s_num_0;
				else if(c == '$') s = s_bulk_0;
				else if(c == '*') s = s_array_0;
				/*
				else if(c == ',') s = s_double_0;	// Double	解析方法类似:，表示浮点数
				else if(c == '_') s = s_null_0;	// Null	表示空
				else if(c == '#') s = s_bool_0;	// Bool	解析方法类似+，区别是把f和t转换为布尔的false和true
				else if(c == '!') s = s_blob_err_0;	// blob error	解析方法类似$，区别是返回的是一种失败。
				else if(c == '(') s = s_big_num_0;	// big num	解析方法类似:，区别内容作看大整数，如果不支持大整数可以临时用字符串替代，但应标记为大整数，到了有支持大整数能力的地方作为大整数使用。
				else if(c == '%') s = s_map_0;	// map	解析方法类似*，区别是结果固定打包成字典，不需要命令提前约束
				else if(c == '~') s = s_set_0;	// set	解析方法类似*，区别是内容视为无序
				else if(c == '|') s = s_attr_0;	// map	解析方法类似%，区别是这个字典不作为结果的一部分，它作为对返回结果的隐含说明。
				*/
				else{
					s = s_str_0;
					tmpstr.push_back(c);
				}
				break;
			case s_str_0:
			case s_err_0:
			case s_num_0:
			case s_array_0:
			case s_bulk_0:
				if(c =='\r') s = (stream_status)(s + 1);
				else tmpstr.push_back(c);
				break;
			case s_str_cr:
				if(c =='\n'){
					data.push_back(tmpstr);
					tmpstr.clear();
					if(array_item_count > 0){
						--array_item_count;
					}
					s = s_0;
				}else return false;
				break;
			case s_err_cr:
				if(c =='\n'){
					data.push_back(tmpstr);
					tmpstr.clear();
					if(array_item_count > 0){
						--array_item_count;
					}
					s = s_0;
				}else return false;
				break;
			case s_num_cr:
				if(c =='\n'){
					char* p;
					lyramilk::data::int64 i = strtoll(tmpstr.c_str(),&p,10);
					data.push_back(i);
					tmpstr.clear();
					if(array_item_count > 0){
						--array_item_count;
					}
					s = s_0;
				}else return false;
				break;
			case s_bulk_cr:
				if(c =='\n'){
					char* p;
					bulk_bytes_count = strtoll(tmpstr.c_str(),&p,10);
					tmpstr.clear();
					s = s_bulk_data;
				}else return false;
				break;
			case s_array_cr:
				if(c =='\n'){
					char* p;
					array_item_count = strtoll(tmpstr.c_str(),&p,10);
					tmpstr.clear();
					s = s_0;
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
				bool r = notify_cmd(data,os);
				data.clear();
				return r;
			}
		}
		return true;
	}

}}