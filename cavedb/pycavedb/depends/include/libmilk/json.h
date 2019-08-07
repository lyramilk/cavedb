#ifndef _lyramilk_data_json_h_
#define _lyramilk_data_json_h_
#include <vector>

#include "var.h"

namespace lyramilk{ namespace data
{
	/*
		@brief json对象
		@details 用来操作一个json对象。
	*/
	class _lyramilk_api_ json
	{
		lyramilk::data::var& v;
	  public:
		json(lyramilk::data::var& o);
		virtual ~json();
		json& operator =(const lyramilk::data::var& o);

		lyramilk::data::string str() const;
		bool str(const lyramilk::data::string& s);

		static bool stringify(const lyramilk::data::var& v,lyramilk::data::string* str);
		static bool parse(const lyramilk::data::string& str,lyramilk::data::var* v);

		static lyramilk::data::string stringify(const lyramilk::data::var& v);
		static lyramilk::data::var parse(const lyramilk::data::string& str);

		lyramilk::data::string static escape(const lyramilk::data::string& s);
		lyramilk::data::string static unescape(const lyramilk::data::string& s);
	};
}}

_lyramilk_api_ std::ostream& operator << (std::ostream& os, const lyramilk::data::json& t);
_lyramilk_api_ std::istream& operator >> (std::istream& is, lyramilk::data::json& t);
#endif
