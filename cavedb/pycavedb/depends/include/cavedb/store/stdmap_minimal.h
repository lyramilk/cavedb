#ifndef _casedb_stdmap_minimal_h_
#define _casedb_stdmap_minimal_h_

#include <libmilk/var.h>
#include <libmilk/thread.h>
#include <tr1/unordered_map>
#include "../store.h"
#include "../store_reader.h"

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class stdmap_minimal:public lyramilk::cave::store,public lyramilk::cave::store_reader
	{
		typedef lyramilk::data::stringdict datamap_type;
		typedef std::tr1::unordered_map<lyramilk::data::string,datamap_type > table_type;
		table_type data;
		mutable lyramilk::threading::mutex_rw lock;
	  protected:
		virtual bool notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset);
		// db
		virtual bool notify_flushdb(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		virtual bool notify_flushall(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		// key
		virtual bool notify_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		virtual bool notify_move(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		virtual bool notify_pexpireat(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		virtual bool notify_persist(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		virtual bool notify_rename(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		// hashmap
		virtual bool notify_hset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);
		virtual bool notify_hdel(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);

	  public:
		stdmap_minimal();
		virtual ~stdmap_minimal();

	  public:
		virtual bool get_sync_info(lyramilk::data::string* replid,lyramilk::data::uint64* offset) const;
		virtual bool hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::string hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::stringdict hgetall(const lyramilk::data::string& key) const;
	};
}}

#endif
