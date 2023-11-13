#include "util.h"

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	


	lyramilk::data::string encode_for_print(const lyramilk::data::string& ret)
	{
		lyramilk::data::string q;
		q.reserve(ret.size() + 32);


		for(lyramilk::data::string::const_iterator it = ret.begin();it!=ret.end();++it){
			if(isprint(*it)){
				q.push_back(*it);
			}else{
				char l = *it & 0xf;
				char h = (*it >> 4) & 0xf;
				q.push_back('\\');
				q.push_back('x');
				q.push_back("0123456789abcdef"[(unsigned int)h]);
				q.push_back("0123456789abcdef"[(unsigned int)l]);
			}
		}


		return q;
	}


}}