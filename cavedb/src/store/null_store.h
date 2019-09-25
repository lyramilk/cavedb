#ifndef _cavedb_null_store_h_
#define _cavedb_null_store_h_

#include <libmilk/var.h>
#include <libmilk/thread.h>
#include "../store.h"
#include "../store_reader.h"

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class null_store:public lyramilk::cave::store,public lyramilk::cave::store_reader
	{
		mutable lyramilk::data::uint64 rspeed_counter;
		mutable lyramilk::data::uint64 rspeed_speed;
		mutable time_t rspeed_tm;
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
		null_store();
		virtual ~null_store();

	  public:
		virtual bool get_sync_info(lyramilk::data::string* replid,lyramilk::data::uint64* offset) const;
		virtual bool hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::string hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::stringdict hgetall(const lyramilk::data::string& key) const;
	};
}}

#endif
