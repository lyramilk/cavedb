#ifndef _lyramilk_data_yaml1_h_
#define _lyramilk_data_yaml1_h_
#include <vector>

#include "var.h"

namespace lyramilk{ namespace data
{
	/*
		@brief yaml对象
		@details 用来操作一个yaml对象。
	*/
	class _lyramilk_api_ yaml
	{
		lyramilk::data::array& ar;
	  public:
		yaml(lyramilk::data::array& o);
		virtual ~yaml();
		yaml& operator =(const lyramilk::data::array& o);

		lyramilk::data::string str() const;
		bool str(lyramilk::data::string s);

		static bool stringify(const lyramilk::data::array& ar,lyramilk::data::string* str);
		static bool parse(lyramilk::data::string str,lyramilk::data::array* m);

		static lyramilk::data::string stringify(const lyramilk::data::array& ar);
		static lyramilk::data::array parse(lyramilk::data::string str);
	};
}}

_lyramilk_api_ std::ostream& operator << (std::ostream& os, const lyramilk::data::yaml& t);
_lyramilk_api_ std::istream& operator >> (std::istream& is, lyramilk::data::yaml& t);
#endif
