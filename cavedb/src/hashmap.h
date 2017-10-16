#ifndef _casedb_casedb_h_
#define _casedb_casedb_h_

#include "slave.h"

namespace lyramilk{ namespace cave
{
	class hashmap:protected hashmap_internal
	{
	  public:
		typedef bool hashmap_scan_callback(const leveldb::Slice& field,const leveldb::Slice& value,void* userdata);

		bool exists(const lyramilk::data::string& field);
		void get(const lyramilk::data::string& field,lyramilk::data::string* value);
		void getall(lyramilk::data::var::map* values);
		void keys(lyramilk::data::strings* values);
		lyramilk::data::int64 len();
		void vals(lyramilk::data::strings* values);
		void scan(lyramilk::data::int64 offset,hashmap_scan_callback,void* userdata);
	};
}}

#endif
