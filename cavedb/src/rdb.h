#ifndef _casedb_rdb_h_
#define _casedb_rdb_h_

#include <libmilk/var.h>

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class rdb
	{
	  protected:
		lyramilk::data::int64 load_integer_value(std::istream& is,int enctype, int flags);
		lyramilk::data::string load_lzf_string(std::istream& is);
		lyramilk::data::uint64 load_length(std::istream& is,bool* isencode);
		unsigned char load_type(std::istream& is);
		lyramilk::data::int32 load_time(std::istream& is);
		lyramilk::data::int64 load_mtime(std::istream& is);
		lyramilk::data::var load_var(std::istream& is);
		lyramilk::data::string load_string(std::istream& is);
		double load_binarydouble(std::istream& is);
		double load_double(std::istream& is);
		void parse_object(std::istream& is,lyramilk::data::uint64 dbid,lyramilk::data::uint64 expiretime,int type,const lyramilk::data::string& key);
	  public:
		rdb();
		virtual ~rdb();
		bool init(const lyramilk::data::string& filename);
		bool init(std::istream& is);
		bool restore(std::istream& is,lyramilk::data::uint64 dbid,lyramilk::data::uint64 expiretime,const lyramilk::data::string& key);

		virtual void notify_select(lyramilk::data::uint64 dbid);
		virtual void notify_aux(const lyramilk::data::string& key,const lyramilk::data::var& value);
		virtual void notify_hset(const lyramilk::data::string& key,const lyramilk::data::string& field,const lyramilk::data::var& value);
		virtual void notify_zadd(const lyramilk::data::string& key,const lyramilk::data::var& value,double score);
		virtual void notify_set(const lyramilk::data::string& key,const lyramilk::data::string& value);
		virtual void notify_rpush(const lyramilk::data::string& key,const lyramilk::data::string& item);
		virtual void notify_sadd(const lyramilk::data::string& key,const lyramilk::data::string& value);
		virtual void notify_pexpireat(const lyramilk::data::string& key,lyramilk::data::uint64 expiretime);
	};
}}

#endif
