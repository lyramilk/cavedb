#define  _CRT_SECURE_NO_WARNINGS
#include "dict.h"
#include "log.h"
#include "json.h"
#include <stdarg.h>
#include <stdio.h>
#include <fstream>

lyramilk::data::dict lyramilk::kdict;

namespace lyramilk{namespace data{

	struct dict_inner_keeper
	{
		bool &inner;
		dict_inner_keeper(bool &inn):inner(inn)
		{
			inner = true;
		}
		~dict_inner_keeper()
		{
			inner = false;
		}
	};

	dict::dict()
	{
		m["无法翻译%s"] = "translate \"%s\" failed.";
		p = NULL;
		inner = false;
	}

	dict::~dict()
	{
	}

	bool dict::load(const lyramilk::data::string& filename)
	{
		if(p && p->load(filename)) return true;

		lyramilk::data::var v;
		{
			std::ifstream ifs(filename.c_str(),std::ifstream::binary|std::ifstream::in);
			lyramilk::data::json j(v);
			ifs >> j;
			ifs.close();
		}
		m = v;
		return true;
	}

	void dict::notify(const lyramilk::data::string& str)
	{
		if(p) {
			p->notify(str);
			return ;
		}
	}

	lyramilk::data::string dict::translate(const lyramilk::data::string& src)
	{
		if(inner){
			return src;
		}
		dict_inner_keeper _(inner);
		if(p) return p->translate(src);
		lyramilk::data::map::iterator it = m.find(src);
		if(it!=m.end()){
			return it->second;
		}

		notify(src);
		return src;
	}

	lyramilk::data::string dict::operator ()(const char* fmt,...)
	{
		char buff[256];
		lyramilk::data::string qfmt = translate(fmt);

		va_list va;
		int cnt;
		va_start(va, fmt);
		cnt = vsnprintf(buff,256, qfmt.c_str(), va);
		va_end(va);
		if(cnt < 256){
			return lyramilk::data::string(buff,cnt);
		}

		std::vector<char> buf(cnt + 1);
		va_start(va, fmt);
		vsprintf(buf.data(),qfmt.c_str(),va);
		va_end(va);
		return lyramilk::data::string(buf.begin(),buf.end());
	}

	lyramilk::data::string dict::trans(const char* fmt,...)
	{
		char buff[256];
		lyramilk::data::string qfmt = translate(fmt);

		va_list va;
		int cnt;
		va_start(va, fmt);
		cnt = vsnprintf(buff,256, qfmt.c_str(), va);
		va_end(va);
		if(cnt < 256){
			return lyramilk::data::string(buff,cnt);
		}

		std::vector<char> buf(cnt + 1);
		va_start(va, fmt);
		vsprintf(buf.data(),qfmt.c_str(),va);
		va_end(va);
		return lyramilk::data::string(buf.begin(),buf.end());
	}

	dict* dict::tie(dict* pdict)
	{
		dict* old = p;
		p = pdict;
		return old;
	}

}}
