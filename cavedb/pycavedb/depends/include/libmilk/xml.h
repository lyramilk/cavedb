#ifndef _lyramilk_data_xml_h_
#define _lyramilk_data_xml_h_
#include <vector>

#include "var.h"

namespace lyramilk{ namespace data
{
	/*
		@brief xml对象
		@details 用来操作一个xml对象。
	*/
	class _lyramilk_api_ xml
	{
		lyramilk::data::map& m;
	  public:
		xml(lyramilk::data::map& o);
		virtual ~xml();
		xml& operator =(const lyramilk::data::map& o);

		lyramilk::data::string str() const;
		bool str(lyramilk::data::string s);

		static bool stringify(const lyramilk::data::map& m,lyramilk::data::string* str);
		static bool parse(lyramilk::data::string str,lyramilk::data::map* m);

		static lyramilk::data::string stringify(const lyramilk::data::map& m);
		static lyramilk::data::map parse(lyramilk::data::string str);

		lyramilk::data::string static escape(const lyramilk::data::string& s);
		lyramilk::data::string static unescape(const lyramilk::data::string& s);
	};
}}

_lyramilk_api_ std::ostream& operator << (std::ostream& os, const lyramilk::data::xml& t);
_lyramilk_api_ std::istream& operator >> (std::istream& is, lyramilk::data::xml& t);
#endif
