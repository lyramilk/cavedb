#include "store.h"
#include "store_reader.h"
#include "rdb.h"
#include <libmilk/log.h>
#include <libmilk/dict.h>
#include <sys/time.h>
#include <map>

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	lyramilk::log::logss static log(lyramilk::klog,"lyramilk.cave.store");

	class rdb_dispatch_for_myself:public rdb
	{
		slave* ps;
	  public:
		rdb_dispatch_for_myself(slave* s)
		{
			ps = s;
		}
		virtual ~rdb_dispatch_for_myself()
		{
		}

		virtual bool notify_hset(const lyramilk::data::string& key,const lyramilk::data::string& field,const lyramilk::data::var& value)
		{
			lyramilk::data::array ar(4);
			ar[0] = "hset";
			ar[1] = key;
			ar[2] = field;
			ar[3] = value;
			return ps->notify_command("?",0,ar);
		}

		virtual bool notify_zadd(const lyramilk::data::string& key,const lyramilk::data::var& value,double score)
		{
			lyramilk::data::array ar(4);
			ar[0] = "zadd";
			ar[1] = key;
			ar[2] = score;
			ar[3] = value;
			return ps->notify_command("?",0,ar);
		}

		virtual bool notify_set(const lyramilk::data::string& key,const lyramilk::data::string& value)
		{
			lyramilk::data::array ar(3);
			ar[0] = "set";
			ar[1] = key;
			ar[2] = value;
			return ps->notify_command("?",0,ar);
		}

		virtual bool notify_rpush(const lyramilk::data::string& key,const lyramilk::data::string& item)
		{
			lyramilk::data::array ar(3);
			ar[0] = "rpush";
			ar[1] = key;
			ar[2] = item;
			return ps->notify_command("?",0,ar);
		}

		virtual bool notify_sadd(const lyramilk::data::string& key,const lyramilk::data::string& value)
		{
			lyramilk::data::array ar(3);
			ar[0] = "sadd";
			ar[1] = key;
			ar[2] = value;
			return ps->notify_command("?",0,ar);
		}
	};

	/// store_reader
	store_reader::store_reader()
	{
		rspeed_speed = 0;
		rspeed_counter = 0;
	}

	store_reader::~store_reader()
	{}


	void store_reader::rspeed_on_read() const
	{
		++rspeed_counter;
		time_t tm_now = time(0);
		if(tm_now != rspeed_tm){
			rspeed_tm = tm_now;
			rspeed_speed = rspeed_counter;
			rspeed_counter = 0;
		}
	}

	lyramilk::data::uint64 store_reader::rspeed() const
	{
		time_t tm_now = time(0);
		if(tm_now != rspeed_tm){
			rspeed_tm = tm_now;
			rspeed_speed = rspeed_counter;
			rspeed_counter = 0;
		}
		return rspeed_speed;
	}

	lyramilk::data::map store_reader::hgetallv(const lyramilk::data::string& key) const
	{
		lyramilk::data::stringdict smap = hgetall(key);
		lyramilk::data::map rmap;
		for(lyramilk::data::stringdict::const_iterator it = smap.begin();it!=smap.end();++it){
			rmap[it->first] = it->second;
		}
		return rmap;
	}

	/// store
	store::store()
	{
		dbid = 0;
		wspeed_speed = 0;
		wspeed_counter = 0;
		wspeed_tm = time(0);
	}

	store::~store()
	{}

	lyramilk::data::int64 store::mstime()
	{
		struct timeval tv;
		gettimeofday(&tv,NULL);
		lyramilk::data::int64 t = tv.tv_sec;
		t *= 1000;
		return t + tv.tv_usec/1000;
	}

	lyramilk::data::uint64 store::wspeed()
	{
		time_t tm_now = time(0);
		if(tm_now != wspeed_tm){
			wspeed_tm = tm_now;
			wspeed_speed = wspeed_counter;
			wspeed_counter = 0;
		}
		return wspeed_speed;
	}

	// db
	bool store::notify_select(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		dbid = args[1];
		return true;
	}

	bool store::notify_ping(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::debug) << "ping" << std::endl;
		return true;
	}

	// key

	bool store::notify_restore(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		lyramilk::data::string key = args[1];
		lyramilk::data::uint64 ttl = args[2];
		lyramilk::data::string data = args[3];
		args[3] = "<序列化值>";//为了日志显示起来顺眼
		lyramilk::data::stringstream is(data);
		rdb_dispatch_for_myself r(this);
		return r.restore(is,dbid,ttl,key);
	}

	bool store::notify_expire(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		lyramilk::data::array ar(3);
		ar[0] = "pexpireat";
		ar[1] = args[1];
		lyramilk::data::int64 expiretime = args[2];
		ar[2] = mstime() + expiretime*1000;
		return notify_pexpireat(replid,offset,ar);

	}

	bool store::notify_expireat(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		lyramilk::data::array ar(3);
		ar[0] = "pexpireat";
		ar[1] = args[1];
		lyramilk::data::int64 expiretime = args[2];
		ar[2] = expiretime*1000;
		return notify_pexpireat(replid,offset,ar);
	}

	bool store::notify_pexpire(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		lyramilk::data::array ar(3);
		ar[0] = "pexpireat";
		ar[1] = args[1];
		lyramilk::data::int64 expiretime = args[2];
		ar[2] = mstime() + expiretime;
		return notify_pexpireat(replid,offset,ar);
	}

	bool store::notify_renamenx(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return notify_rename(replid,offset,args);
	}

	// string
	bool store::notify_append(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","append") << args << std::endl;
		return false;
	}

	bool store::notify_bitor(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","bitor") << args << std::endl;
		return false;
	}

	bool store::notify_getset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","getset") << args << std::endl;
		return false;
	}

	bool store::notify_decr(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","decr") << args << std::endl;
		return false;
	}

	bool store::notify_decrby(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","decrby") << args << std::endl;
		return false;
	}

	bool store::notify_incr(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","incr") << args << std::endl;
		return false;
	}

	bool store::notify_incrby(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","incrby") << args << std::endl;
		return false;
	}

	bool store::notify_mset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","mset") << args << std::endl;
		return false;
	}

	bool store::notify_msetnx(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","msetnx") << args << std::endl;
		return false;
	}

	bool store::notify_psetex(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","psetex") << args << std::endl;
		return false;
	}

	bool store::notify_setex(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","setex") << args << std::endl;
		return false;
	}

	bool store::notify_set(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","set") << args << std::endl;
		return false;
	}

	bool store::notify_setnx(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","setnx") << args << std::endl;
		return false;
	}

	bool store::notify_setbit(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","setbit") << args << std::endl;
		return false;
	}

	bool store::notify_setrange(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","setrange") << args << std::endl;
		return false;
	}

	// hashmap
	bool store::notify_hset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","hset") << args << std::endl;
		return false;
	}

	bool store::notify_hsetnx(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","hsetnx") << args << std::endl;
		return false;
	}

	bool store::notify_hdel(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","hdel") << args << std::endl;
		return false;
	}

	bool store::notify_hincrby(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","hincrby") << args << std::endl;
		return false;
	}

	bool store::notify_hmset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","hmset") << args << std::endl;
		return false;
	}


	// set
	bool store::notify_sadd(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","sadd") << args << std::endl;
		return false;
	}

	bool store::notify_sdiffstore(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","sdiffstore") << args << std::endl;
		return false;
	}

	bool store::notify_sinterstore(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","sinterstore") << args << std::endl;
		return false;
	}

	bool store::notify_smove(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","smove") << args << std::endl;
		return false;
	}

	bool store::notify_srem(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","srem") << args << std::endl;
		return false;
	}

	bool store::notify_sunionstore(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","sunionstore") << args << std::endl;
		return false;
	}

	// list
	bool store::notify_zadd(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","zadd") << args << std::endl;
		return false;
	}
	bool store::notify_zincrby(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","zincrby") << args << std::endl;
		return false;
	}
	bool store::notify_zrem(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","zrem") << args << std::endl;
		return false;
	}
	bool store::notify_zremrangebyrank(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","zremrangebyrank") << args << std::endl;
		return false;
	}
	bool store::notify_zremrangebyscore(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","zremrangebyscore") << args << std::endl;
		return false;
	}
	bool store::notify_zunionstore(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","zunionstore") << args << std::endl;
		return false;
	}
	bool store::notify_zinterstore(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","zinterstore") << args << std::endl;
		return false;
	}

	// list
	bool store::notify_blpop(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","blpop") << args << std::endl;
		return false;
	}

	bool store::notify_brpop(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","brpop") << args << std::endl;
		return false;
	}

	bool store::notify_brpoplpush(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","brpoplpush") << args << std::endl;
		return false;
	}

	bool store::notify_linsert(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","linsert") << args << std::endl;
		return false;
	}

	bool store::notify_lpop(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","lpop") << args << std::endl;
		return false;
	}

	bool store::notify_lpush(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","lpush") << args << std::endl;
		return false;
	}

	bool store::notify_lpushx(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","lpushx") << args << std::endl;
		return false;
	}

	bool store::notify_lrem(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","lrem") << args << std::endl;
		return false;
	}

	bool store::notify_lset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","lset") << args << std::endl;
		return false;
	}

	bool store::notify_ltrim(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","ltrim") << args << std::endl;
		return false;
	}

	bool store::notify_rpop(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","rpop") << args << std::endl;
		return false;
	}

	bool store::notify_rpoplpush(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","rpoplpush") << args << std::endl;
		return false;
	}

	bool store::notify_rpush(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","rpush") << args << std::endl;
		return false;
	}

	bool store::notify_rpushx(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","rpushx") << args << std::endl;
		return false;
	}

	bool store::notify_ssdb_qset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","ssdb_qset") << args << std::endl;
		return false;
	}

	bool store::notify_ssdb_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		log(lyramilk::log::warning) << D("%s未实现","ssdb_del") << args << std::endl;
		return false;
	}


	bool store::notify_sync_start(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return notify_flushall(replid,offset,args);
	}

	bool store::notify_sync_continue(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return notify_psync(replid,offset);
	}

	bool store::notify_sync_overflow(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		return notify_flushall(replid,offset,args);
	}

	typedef bool (*store_event_callback)(store* pthis,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args);

	#define define_selector(mm) bool static cbk_notify_##mm(store* pthis,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args){return pthis->notify_##mm(replid,offset,args);}
	class store_dispatcher{
	  public:
		define_selector(append);
		define_selector(bitor);
		define_selector(blpop);
		define_selector(brpop);
		define_selector(brpoplpush);
		define_selector(decr);
		define_selector(decrby);
		define_selector(del);
		define_selector(expire);
		define_selector(expireat);
		define_selector(flushall);
		define_selector(flushdb);
		define_selector(getset);
		define_selector(hdel);
		define_selector(hincrby);
		define_selector(hmset);
		define_selector(hset);
		define_selector(hsetnx);
		define_selector(incr);
		define_selector(incrby);
		define_selector(linsert);
		define_selector(lpop);
		define_selector(lpush);
		define_selector(lpushx);
		define_selector(lrem);
		define_selector(lset);
		define_selector(ltrim);
		define_selector(move);
		define_selector(mset);
		define_selector(msetnx);
		define_selector(persist);
		define_selector(pexpire);
		define_selector(pexpireat);
		define_selector(ping);
		define_selector(psetex);
		define_selector(rename);
		define_selector(renamenx);
		define_selector(restore);
		define_selector(rpop);
		define_selector(rpoplpush);
		define_selector(rpush);
		define_selector(rpushx);
		define_selector(sadd);
		define_selector(sdiffstore);
		define_selector(select);
		define_selector(set);
		define_selector(setbit);
		define_selector(setex);
		define_selector(setnx);
		define_selector(setrange);
		define_selector(sinterstore);
		define_selector(smove);
		define_selector(srem);
		define_selector(sunionstore);
		define_selector(zadd);
		define_selector(zincrby);
		define_selector(zrem);
		define_selector(zremrangebyrank);
		define_selector(zremrangebyscore);
		define_selector(zunionstore);
		define_selector(zinterstore);
		define_selector(ssdb_qset);
		define_selector(ssdb_del);
		define_selector(sync_start);
		define_selector(sync_continue);
		define_selector(sync_overflow);
	};

	#define init_selector(mm) m[#mm] = &store_dispatcher::cbk_notify_##mm
	std::map<lyramilk::data::string,store_event_callback> init_dispatcher()
	{
		std::map<lyramilk::data::string,store_event_callback> m;
		init_selector(append);
		init_selector(bitor);
		init_selector(blpop);
		init_selector(brpop);
		init_selector(brpoplpush);
		init_selector(decr);
		init_selector(decrby);
		init_selector(del);
		init_selector(expire);
		init_selector(expireat);
		init_selector(flushall);
		init_selector(flushdb);
		init_selector(getset);
		init_selector(hdel);
		init_selector(hincrby);
		init_selector(hmset);
		init_selector(hset);
		init_selector(hsetnx);
		init_selector(incr);
		init_selector(incrby);
		init_selector(linsert);
		init_selector(lpop);
		init_selector(lpush);
		init_selector(lpushx);
		init_selector(lrem);
		init_selector(lset);
		init_selector(ltrim);
		init_selector(move);
		init_selector(mset);
		init_selector(msetnx);
		init_selector(persist);
		init_selector(pexpire);
		init_selector(pexpireat);
		init_selector(ping);
		init_selector(psetex);
		init_selector(rename);
		init_selector(renamenx);
		init_selector(restore);
		init_selector(rpop);
		init_selector(rpoplpush);
		init_selector(rpush);
		init_selector(rpushx);
		init_selector(sadd);
		init_selector(sdiffstore);
		init_selector(select);
		init_selector(set);
		init_selector(setbit);
		init_selector(setex);
		init_selector(setnx);
		init_selector(setrange);
		init_selector(sinterstore);
		init_selector(smove);
		init_selector(srem);
		init_selector(sunionstore);
		init_selector(zadd);
		init_selector(zincrby);
		init_selector(zrem);
		init_selector(zremrangebyrank);
		init_selector(zremrangebyscore);
		init_selector(zunionstore);
		init_selector(zinterstore);
		init_selector(ssdb_qset);
		init_selector(ssdb_del);
		init_selector(sync_start);
		init_selector(sync_continue);
		init_selector(sync_overflow);
		return m;
	}
	std::map<lyramilk::data::string,store_event_callback> dispatch_map = init_dispatcher();

	bool store::notify_command(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		++wspeed_counter;
		time_t tm_now = time(0);
		if(tm_now != wspeed_tm){
			wspeed_tm = tm_now;
			wspeed_speed = wspeed_counter;
			wspeed_counter = 0;
		}

		lyramilk::data::string cmd = args[0];
		std::map<lyramilk::data::string,store_event_callback>::const_iterator it = dispatch_map.find(cmd);
		if(it!=dispatch_map.end()){
			return it->second(this,replid,offset,args);
		}else{
			log(lyramilk::log::error) << D("未实现命令",cmd.c_str()) << args << std::endl;
		}

		return false;
	}

	bool store::notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
	{
		return true;
	}
}}

/*
append
bitor
blpop
brpop
brpoplpush
decr
decrby
del
expire
expireat
flushall
flushdb
getset
hdel
hincrby
hmset
hset
hsetnx
incr
incrby
linsert
lpop
lpush
lpushx
lrem
lset
ltrim
move
mset
msetnx
persist
pexpire
pexpireat
ping
psetex
rename
renamenx
restore
rpop
rpoplpush
rpush
rpushx
sadd
sdiffstore
select
set
setbit
setex
setnx
setrange
sinterstore
smove
srem
sunionstore
*/