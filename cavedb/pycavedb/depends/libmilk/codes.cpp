#include "codes.h"
#include "log.h"
#include "dict.h"

#include <cassert>
#include <algorithm>
#include <cctype>

#include <errno.h>
#include <string.h>

#ifdef LZ4_FOUND
	#include <lz4.h>
#endif

#ifdef __linux__
	#include <iconv.h>
#endif

namespace lyramilk{ namespace data
{
	coding_exception::coding_exception(const lyramilk::data::string& msg):lyramilk::exception(msg)
	{}

#ifdef __linux__

	lyramilk::data::string iconv(const lyramilk::data::string& str,const lyramilk::data::string& from,const lyramilk::data::string& to)
	{
		const size_t ds = 65536;
		iconv_t cd = iconv_open(to.c_str(),from.c_str());
		if(cd == 0 || cd == (iconv_t)-1){
			throw lyramilk::data::coding_exception(D("不支持的iconv编码：%s,%s-->%s",strerror(errno),from.c_str(),to.c_str()));
			return "";
		}
		char* p1 = (char*)str.c_str();
		size_t p1s = str.size();
		std::vector<char> ret;
		char* p2 = ret.data();
		size_t p2s = ret.size();

		int rc = -1;
		do{
			size_t pos = p2 - ret.data();
			ret.insert(ret.end(),ds,0);
			p2 = ret.data() + pos;
			p2s = ret.size() - pos;
			rc = ::iconv(cd,&p1,&p1s,&p2,&p2s);
		}while(rc == -1 && p2s < ds);

		iconv_close(cd);
		if(rc == -1){
			throw lyramilk::data::coding_exception(D("编码转换错误：%s",strerror(errno)));
			return "";
		}
		return lyramilk::data::string(ret.data(),p2 - (char*)ret.data());
	}
#endif

	lyramilk::data::coding::~coding()
	{}

	lyramilk::data::codes* lyramilk::data::codes::instance()
	{
		static lyramilk::data::codes _mm;
		return &_mm;
	}

	bool lyramilk::data::codes::define(const lyramilk::data::string& codingname,getter ctr)
	{
		lyramilk::data::string str(codingname.size(),0);
		transform(codingname.begin(), codingname.end(), str.begin(), tolower);
		std::pair<builder_type::iterator,bool> it = builder.insert(std::make_pair(str,ctr));
		return it.second;
	}

	bool lyramilk::data::codes::undefine(const lyramilk::data::string& codingname)
	{
		lyramilk::data::string str(codingname.size(),0);
		transform(codingname.begin(), codingname.end(), str.begin(), tolower);
		builder_type::iterator it = builder.find(str);
		if(it == builder.end()) return false;
		builder.erase(it);
		return true;
	}

	coding* lyramilk::data::codes::getcoder(const string& codingname)
	{
		lyramilk::data::string str(codingname.size(),0);
		transform(codingname.begin(), codingname.end(), str.begin(), tolower);
		builder_type::iterator it = builder.find(str);
		if(it == builder.end()) return nullptr;
		return it->second();
	}

	lyramilk::data::string lyramilk::data::codes::encode(const lyramilk::data::string& codingname,const lyramilk::data::string& src) throw (lyramilk::exception)
	{
		lyramilk::data::string str(codingname.size(),0);
		transform(codingname.begin(), codingname.end(), str.begin(), tolower);
		builder_type::iterator it = builder.find(str);
		if(it == builder.end()){
			throw lyramilk::data::coding_exception(D("不可识别的编码：%s",codingname.c_str()));
		}
		return it->second()->encode(src);
	}
	lyramilk::data::string lyramilk::data::codes::decode(const lyramilk::data::string& codingname,const lyramilk::data::string& src) throw (lyramilk::exception)
	{
		lyramilk::data::string str(codingname.size(),0);
		transform(codingname.begin(), codingname.end(), str.begin(), tolower);
		builder_type::iterator it = builder.find(str);
		if(it == builder.end()){
			throw lyramilk::data::coding_exception(D("不可识别的编码：%s",codingname.c_str()));
		}
		return it->second()->decode(src);
	}

	lyramilk::data::array lyramilk::data::codes::supports()
	{
		lyramilk::data::array r;
		builder_type::iterator it = builder.begin();
		for(;it!=builder.end();++it){
			r.push_back(it->first);
		}
		return r;
	}

#ifdef LZ4_FOUND
	/*************** coding_lz4 ******************/
	class coding_lz4:public lyramilk::data::coding
	{
	  public:
		virtual lyramilk::data::string decode(const lyramilk::data::string& str)
		{
			std::vector<char> buf(str.size());
			while(true){
				int i = LZ4_decompress_safe(str.c_str(),buf.data(),str.size(),buf.size());
				if(i > 0){
					buf.erase(buf.begin()+i,buf.end());
					break;
				}
				buf.resize(buf.size()*2);
			}
			return lyramilk::data::string(buf.data(),buf.size());
		}
		virtual lyramilk::data::string encode(const lyramilk::data::string& str)
		{
			std::vector<char> buf(str.size());
			while(true){
				int i = LZ4_compress_fast(str.c_str(),buf.data(),str.size(),buf.size(),3);
				if(i > 0){
					buf.erase(buf.begin()+i,buf.end());
					break;
				}
				buf.resize(buf.size()*2);
			}
			return lyramilk::data::string(buf.data(),buf.size());
		}

		static lyramilk::data::coding* getter()
		{
			static coding_lz4 _mm;
			return &_mm;
		}
	};
#endif

class coding_utf8:public lyramilk::data::coding
{
  public:
	virtual lyramilk::data::string decode(const lyramilk::data::string& str)
	{
		return str;
	}
	virtual lyramilk::data::string encode(const lyramilk::data::string& str)
	{
		return str;
	}

	static lyramilk::data::coding* getter()
	{
		static coding_utf8 _mm;
		return &_mm;
	}
};


template <char* name>
class coding_t:public lyramilk::data::coding
{
  public:
	virtual lyramilk::data::string decode(const lyramilk::data::string& str)
	{
		return iconv(str,name,"UTF-8");
	}
	virtual lyramilk::data::string encode(const lyramilk::data::string& str)
	{
		return iconv(str,"UTF-8",name);
	}

	static lyramilk::data::coding* getter()
	{
		static coding_t _mm;
		return &_mm;
	}
};

char c_gbk[] = "gbk";
char c_gb2312[] = "gb2312";
char c_GB2312[] = "GB2312";
char c_big5[] = "big5";
char c_utf16[] = "utf16le";
char c_utf32[] = "utf32";


static bool ___init()
{
#ifdef LZ4_FOUND
	lyramilk::data::codes::instance()->define("lz4",coding_lz4::getter);
#endif
	lyramilk::data::codes::instance()->define(c_gbk,coding_t<c_gbk>::getter);
	lyramilk::data::codes::instance()->define(c_gb2312,coding_t<c_GB2312>::getter);
	lyramilk::data::codes::instance()->define(c_GB2312,coding_t<c_GB2312>::getter);
	lyramilk::data::codes::instance()->define(c_big5,coding_t<c_big5>::getter);
	lyramilk::data::codes::instance()->define(c_utf16,coding_t<c_utf16>::getter);
	lyramilk::data::codes::instance()->define("utf-16",coding_t<c_utf16>::getter);
	lyramilk::data::codes::instance()->define("utf16",coding_t<c_utf16>::getter);
	lyramilk::data::codes::instance()->define(c_utf32,coding_t<c_utf32>::getter);
	lyramilk::data::codes::instance()->define("utf-32",coding_t<c_utf32>::getter);
	lyramilk::data::codes::instance()->define("wchar_t",coding_t<c_utf32>::getter);
	lyramilk::data::codes::instance()->define("utf8",coding_utf8::getter);
	lyramilk::data::codes::instance()->define("utf-8",coding_utf8::getter);
	lyramilk::data::codes::instance()->define("UTF8",coding_utf8::getter);
	lyramilk::data::codes::instance()->define("UTF-8",coding_utf8::getter);
	return true;
}

#ifdef __GNUC__
static __attribute__ ((constructor)) void __init()
{
	___init();
}
#else
bool r = __init();
#endif

}}
