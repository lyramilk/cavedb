#ifndef _casedb_dense_hash_map_minimal_h_
#define _casedb_dense_hash_map_minimal_h_

#include <libmilk/var.h>
#include <libmilk/thread.h>
#include "../store.h"
#include "../store_reader.h"

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class dense_hash_map_minimal:public lyramilk::cave::store,public lyramilk::cave::store_reader
	{
		void* data;
		mutable lyramilk::threading::mutex_rw lock;
	  protected:
		virtual bool notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset);
		// db
		virtual void notify_flushdb(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_flushall(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		// key
		virtual void notify_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_move(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_pexpireat(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_persist(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_rename(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		// hashmap
		virtual void notify_hset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_hdel(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);

	  public:
		dense_hash_map_minimal();
		virtual ~dense_hash_map_minimal();

	  public:
		virtual bool get_sync_info(lyramilk::data::string* replid,lyramilk::data::uint64* offset) const;
		virtual bool hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::string hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual lyramilk::data::stringdict hgetall(const lyramilk::data::string& key) const;
	};
}}

#endif
