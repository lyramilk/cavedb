#include "resp.h"
#include <libmilk/json.h>

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{


	resp_result resp23_from_stream(std::istream& is,lyramilk::data::var* ret)
	{
		char c = is.get();
		switch(c){
		case '+':{
			char c = is.get();
			lyramilk::data::string stmp;
			for(;c != '\r';c = is.get()){
				stmp.push_back(c);
			}
			if(is.get() != '\n') {
				return resp_parse_error;
			}

			*ret = stmp;
			return resp_data;
		}break;
		case '-':{
			char c = is.get();
			lyramilk::data::string stmp;
			for(;c != '\r';c = is.get()){
				stmp.push_back(c);
			}
			if(is.get() != '\n') {
				return resp_parse_error;
			}

			*ret = stmp;
			return resp_msg_error;
		}break;
		case ':':{
			char c = is.get();
			lyramilk::data::string stmp;
			for(;c != '\r';c = is.get()){
				if(c <= '9' && c >= '0'){
					stmp.push_back(c);
				}else{
					return resp_parse_error;
				}
			}
			if(is.get() != '\n') {
				return resp_parse_error;
			}

			char *p;
			*ret = strtoll(stmp.c_str(),&p,10);
			return resp_data;
		}break;
		case '$':{
			char c = is.get();
			lyramilk::data::string slen;
			for(;c != '\r';c = is.get()){
				if(c <= '9' && c >= '0'){
					slen.push_back(c);
				}else{
					return resp_parse_error;
				}
			}
			if(is.get() != '\n') {
				return resp_parse_error;
			}

			char *p;
			long long len = strtoll(slen.c_str(),&p,10);

			lyramilk::data::string str;
			str.reserve(len);
			for(long long i=0;i<len;++i){
				str.push_back(is.get());
				if(!is.good()) {
				return resp_parse_error;
			}
			}
			if(is.get() != '\r') {
				return resp_parse_error;
			}
			if(is.get() != '\n') {
				return resp_parse_error;
			}
			*ret = str;
			return resp_data;
		}break;
		case '*':{
			char c = is.get();
			lyramilk::data::string stmp;
			for(;c != '\r';c = is.get()){
				if(c <= '9' && c >= '0'){
					stmp.push_back(c);
				}else{
					return resp_parse_error;
				}
			}
			if(is.get() != '\n') {
				return resp_parse_error;
			}
			ret->type(lyramilk::data::var::t_array);
			lyramilk::data::array& ar = *ret;

			char *p;
			long long datacount = strtoll(stmp.c_str(),&p,10);
			ar.resize(datacount);
			for(long long i=0;i<datacount;++i){
				resp_result rr = resp23_from_stream(is,&ar[i]);
				if(rr != resp_data ){
					return rr;
				}
			}
			return resp_data;
		}break;
		case ',':{
			char c = is.get();
			lyramilk::data::string stmp;
			for(;c != '\r';c = is.get()){
				if(c <= '9' && c >= '0'){
					stmp.push_back(c);
				}else{
					return resp_parse_error;
				}
			}
			if(is.get() != '\n') {
				return resp_parse_error;
			}

			char *p;
			*ret = strtod(stmp.c_str(),&p);
			return resp_data;
		}break;
		case '_':{
			if(is.get() != '\r') {
				return resp_parse_error;
			}
			if(is.get() != '\n') {
				return resp_parse_error;
			}
			ret->clear();
			return resp_data;
		}break;
		case '#':{
			char c = is.get();
			if(is.get() != '\r') {
				return resp_parse_error;
			}
			if(is.get() != '\n') {
				return resp_parse_error;
			}
			if(c == 't'){
				*ret = true;
				return resp_data;
			}
			if(c == 'f'){
				*ret = true;
				return resp_data;
			}
			return resp_parse_error;
		}break;
		case '!':{
			char c = is.get();
			lyramilk::data::string slen;
			for(;c != '\r';c = is.get()){
				if(c <= '9' && c >= '0'){
					slen.push_back(c);
				}else{
					return resp_parse_error;
				}
			}
			if(is.get() != '\n') {
				return resp_parse_error;
			}

			char *p;
			long long len = strtoll(slen.c_str(),&p,10);

			lyramilk::data::string str;
			str.reserve(len);
			for(long long i=0;i<len;++i){
				str.push_back(is.get());
				if(!is.good()) {
				return resp_parse_error;
			}
			}
			if(is.get() != '\r') {
				return resp_parse_error;
			}
			if(is.get() != '\n') {
				return resp_parse_error;
			}
			*ret = str;
			return resp_msg_error;
		}break;
		case '(':{
			char c = is.get();
			lyramilk::data::string stmp;
			for(;c != '\r';c = is.get()){
				if(c <= '9' && c >= '0'){
					stmp.push_back(c);
				}else{
					return resp_parse_error;
				}
			}
			if(is.get() != '\n') {
				return resp_parse_error;
			}

			//大整数,不应该识别为str,但是var不支持大整数,就这么用了.
			*ret = stmp;
			return resp_data;
		}break;
		case '%':{
			char c = is.get();
			lyramilk::data::string stmp;
			for(;c != '\r';c = is.get()){
				if(c <= '9' && c >= '0'){
					stmp.push_back(c);
				}else{
					return resp_parse_error;
				}
			}
			if(is.get() != '\n') {
				return resp_parse_error;
			}
			ret->type(lyramilk::data::var::t_map);
			lyramilk::data::map& m = *ret;

			char *p;
			long long datacount = strtoll(stmp.c_str(),&p,10) >> 1;
			for(long long i=0;i<datacount;++i){
				lyramilk::data::var key;
				resp_result rr = resp23_from_stream(is,&key);
				if(rr != resp_data ){
					return rr;
				}

				rr = resp23_from_stream(is,&m[key.str()]);
				if(rr != resp_data ){
					return rr;
				}
			}
			return resp_data;
		}break;
		case '~':{
			char c = is.get();
			lyramilk::data::string stmp;
			for(;c != '\r';c = is.get()){
				if(c <= '9' && c >= '0'){
					stmp.push_back(c);
				}else{
					return resp_parse_error;
				}
			}
			if(is.get() != '\n') {
				return resp_parse_error;
			}
			ret->type(lyramilk::data::var::t_array);
			lyramilk::data::array& ar = *ret;

			char *p;
			long long datacount = strtoll(stmp.c_str(),&p,10);
			ar.resize(datacount);
			for(long long i=0;i<datacount;++i){
				resp_result rr = resp23_from_stream(is,&ar[i]);
				if(rr != resp_data ){
					return rr;
				}
			}
			return resp_data;
		}break;
		case '|':{
			char c = is.get();
			lyramilk::data::string stmp;
			for(;c != '\r';c = is.get()){
				if(c <= '9' && c >= '0'){
					stmp.push_back(c);
				}else{
					return resp_parse_error;
				}
			}
			if(is.get() != '\n') {
				return resp_parse_error;
			}
			ret->type(lyramilk::data::var::t_map);
			lyramilk::data::map& m = *ret;

			char *p;
			long long datacount = strtoll(stmp.c_str(),&p,10) >> 1;
			for(long long i=0;i<datacount;++i){
				lyramilk::data::var key;
				resp_result rr = resp23_from_stream(is,&key);
				if(rr != resp_data ){
					return rr;
				}

				rr = resp23_from_stream(is,&m[key.str()]);
				if(rr != resp_data ){
					return rr;
				}
			}
			return resp_hidden_data;
		}break;
		}
		return resp_parse_error;
	}


	bool resp23_to_stream(const lyramilk::data::var& ret,lyramilk::data::ostream& os)
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
		}else if(ret.type() == lyramilk::data::var::t_uint){
			os << ":" << (unsigned long long)ret << "\r\n";
			return true;
		}else if(ret.type() == lyramilk::data::var::t_int){
			os << ":" << (long long)ret << "\r\n";
			return true;
		}else if(ret.type() == lyramilk::data::var::t_array){
			const lyramilk::data::array& ar = ret;
			os << "*" << ar.size() << "\r\n";
			for(lyramilk::data::array::const_iterator it = ar.begin();it !=ar.end();++it){
				if(!resp23_to_stream(*it,os)) return false;
			}
			return true;
		}else if(ret.type() == lyramilk::data::var::t_map){
			const lyramilk::data::map& ar = ret;
			os << "*" << (ar.size() << 1) << "\r\n";
			for(lyramilk::data::map::const_iterator it = ar.begin();it !=ar.end();++it){
				if(!resp23_to_stream(it->first,os)) return false;
				if(!resp23_to_stream(it->second,os)) return false;
			}
			return true;
		}else if(ret.type() == lyramilk::data::var::t_invalid){
			os << "$-1\r\n";
			return true;
		}else{
			return false;
		}
	}

	bool resp23_to_stream(const lyramilk::data::array& ar,lyramilk::data::ostream& os)
	{
		os << "*" << ar.size() << "\r\n";
		for(lyramilk::data::array::const_iterator it = ar.begin();it !=ar.end();++it){
			if(!resp23_to_stream(*it,os)) return false;
		}
		return true;
	}

	bool resp23_to_stream(const lyramilk::data::map& ar,lyramilk::data::ostream& os)
	{
		os << "*" << (ar.size() << 1) << "\r\n";
		for(lyramilk::data::map::const_iterator it = ar.begin();it !=ar.end();++it){
			if(!resp23_to_stream(it->first,os)) return false;
			if(!resp23_to_stream(it->second,os)) return false;
		}
		return true;
	}

	// resp23_as_session

	resp23_as_session::resp23_as_session()
	{
		s = s_0;
	}

	resp23_as_session::~resp23_as_session()
	{
		
	}

	bool resp23_as_session::oninit(lyramilk::data::ostream& os)
	{
		return true;
	}



	bool resp23_as_session::output_redis_result(lyramilk::cave::cmdstatus rs,const lyramilk::data::var& ret,lyramilk::data::ostream& os)
	{
		if(rs == lyramilk::cave::cs_data){
			return resp23_to_stream(ret,os);
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