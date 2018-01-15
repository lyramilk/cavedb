#ifndef _casedb_stdmap_minimal_h_
#define _casedb_stdmap_minimal_h_

#include <libmilk/var.h>
#include <libmilk/thread.h>
#include "store.h"

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class stdmap_minimal:public lyramilk::cave::store
	{
		struct mapeddata
		{
			lyramilk::data::int64 expire;
			std::map<std::string,std::string> data;
			mutable lyramilk::threading::mutex_rw lock;
		};
		std::map<std::string,mapeddata> data;
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
		stdmap_minimal();
		virtual ~stdmap_minimal();
		virtual lyramilk::data::string hget(const lyramilk::data::string& key,const lyramilk::data::string& field);
		virtual lyramilk::data::var::map hgetall(const lyramilk::data::string& key);
	};
}}

#endif
