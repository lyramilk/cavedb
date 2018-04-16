#ifndef _casedb_store_reader_h_
#define _casedb_store_reader_h_

#include <libmilk/var.h>
#include <libmilk/thread.h>

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class store_reader
	{
	  public:
		virtual bool get_sync_info(lyramilk::data::string* replid,lyramilk::data::uint64* offset) const = 0;
		virtual bool hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const = 0;
		virtual lyramilk::data::string hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const = 0;
		virtual lyramilk::data::var::map hgetall(const lyramilk::data::string& key) const = 0;
	};
}}

#endif
