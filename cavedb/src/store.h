#ifndef _casedb_store_h_
#define _casedb_store_h_

#include <libmilk/var.h>
#include <libmilk/iterator.h>
#include "slave.h"
#include "slice.h"

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	//std::map<double,lyramilk::data::string> zpairs;

	class store:public lyramilk::cave::slave
	{
		friend class store_dispatcher;
		lyramilk::data::uint64 dbid;
		lyramilk::data::uint64 wspeed_counter;
		lyramilk::data::uint64 wspeed_speed;
		time_t wspeed_tm;
		virtual void notify_command(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual bool notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset);
	  protected:
		store();
		lyramilk::data::int64 mstime();
		lyramilk::data::uint64 inline current_dbindex(){return dbid;}
	  protected:
		// db
		virtual void notify_flushdb(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args) = 0;
		virtual void notify_flushall(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args) = 0;
		virtual void notify_select(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_ping(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		// key
		virtual void notify_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args) = 0;
		virtual void notify_move(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args) = 0;
		virtual void notify_restore(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_expire(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_expireat(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_pexpire(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_pexpireat(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args) = 0;
		virtual void notify_persist(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args) = 0;
		virtual void notify_rename(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args) = 0;
		virtual void notify_renamenx(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		
		// string
		virtual void notify_append(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_bitor(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_getset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_decr(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_decrby(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_incr(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_incrby(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_mset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_msetnx(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_psetex(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_setex(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_set(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_setnx(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_setbit(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_setrange(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);

		// hashmap
		virtual void notify_hset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_hsetnx(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_hdel(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_hincrby(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_hmset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);

		// set
		virtual void notify_sadd(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_sdiffstore(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_sinterstore(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_smove(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_srem(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_sunionstore(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);

		// list
		virtual void notify_blpop(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_brpop(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_brpoplpush(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_linsert(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_lpop(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_lpush(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_lpushx(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_lrem(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_lset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_ltrim(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_rpop(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_rpoplpush(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_rpush(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual void notify_rpushx(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
	  public:
		virtual lyramilk::data::uint64 wspeed();
		virtual ~store();
	};
}}

#endif
