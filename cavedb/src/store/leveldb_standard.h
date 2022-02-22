#ifndef _cavedb_leveldb_standard_h_
#define _cavedb_leveldb_standard_h_

#include <libmilk/var.h>
#include <libmilk/iterator.h>
#include <libmilk/atom.h>
#include <libmilk/netmonitor.h>

#include "../store.h"
#include "../store_reader.h"
#include "../slice.h"
#include "../redis_pack.h"
#include "../redis_like_session.h"
#include "leveldb_minimal_adapter.h"

namespace leveldb{class DB;};

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{


	class leveldb_standard : /*public lyramilk::cave::store , public lyramilk::cave::store_reader,*/public minimal_interface
	{
	  protected:
		leveldb::DB* ldb;
		virtual lyramilk::data::string hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		static void* thread_auto_compact(leveldb::DB* ldb);
	  protected:
		virtual bool notify_idle(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata);
		virtual bool notify_psync(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata);
		// db
		virtual bool notify_flushdb(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_flushall(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		// key
		virtual bool notify_del(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_move(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_pexpireat(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_persist(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_rename(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		// hashmap
		virtual bool notify_hset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_hmset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_hdel(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		// kv
		virtual bool notify_set(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_ssdb_del(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		// queue
		virtual bool notify_ssdb_qset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_lpop(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_rpop(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		// zset
		virtual bool notify_zadd(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_zrem(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		// set
		virtual bool notify_sadd(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);
		virtual bool notify_srem(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata);

	  public:
		const static std::string cfver;

		leveldb_standard();
		virtual ~leveldb_standard();

		static minimal_interface* open(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing);
		bool open_leveldb(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing = false);
	  public:
		virtual bool compact();
		virtual lyramilk::data::string get_property(const lyramilk::data::string& property);
		virtual bool is_on_full_sync();
	  public:
		virtual bool get_sync_info(const lyramilk::data::string& masterid,lyramilk::data::string* replid,lyramilk::data::uint64* offset) const;


		virtual bool zrange(const lyramilk::data::string& key,lyramilk::data::int64 start,lyramilk::data::int64 stop,bool withscore,lyramilk::data::strings* result) const;
		virtual lyramilk::data::string zscan(const lyramilk::data::string& key,const lyramilk::data::string& current,lyramilk::data::uint64 count,lyramilk::data::strings* result) const;
		virtual lyramilk::data::uint64 zcard(const lyramilk::data::string& key) const;

		virtual lyramilk::data::string sscan(const lyramilk::data::string& key,const lyramilk::data::string& current,lyramilk::data::uint64 count,lyramilk::data::strings* result) const;
		virtual bool spop(const lyramilk::data::string& key,lyramilk::data::string* result) const;
		virtual lyramilk::data::uint64 scard(const lyramilk::data::string& key) const;

		virtual bool get(const lyramilk::data::string& key,lyramilk::data::string* value) const;

		virtual bool hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const;
		virtual bool hget(const lyramilk::data::string& key,const lyramilk::data::string& field,lyramilk::data::string* value) const;
		virtual lyramilk::data::stringdict hgetall(const lyramilk::data::string& key) const;
		virtual lyramilk::data::string hscan(const lyramilk::data::string& key,const lyramilk::data::string& current,lyramilk::data::uint64 count,lyramilk::data::strings* result) const;
		virtual lyramilk::data::uint64 hlen(const lyramilk::data::string& key) const;

		virtual lyramilk::data::string scan(const lyramilk::data::string& current,lyramilk::data::uint64 count,lyramilk::data::strings* result) const;
		virtual lyramilk::data::string type(const lyramilk::data::string& key) const;
	  protected:
		std::map<lyramilk::data::string,lyramilk::netio::aiomonitor* > amons;
		lyramilk::threading::mutex_rw channel_amons_lock;
	  public:
		virtual bool subscribe(int fd,const lyramilk::data::string& channel);
		virtual bool publish(const lyramilk::data::string& channel,const lyramilk::data::string& message);
	};





	class leveldb_standard_redislike_session : public redislike_session
	{
		std::map<lyramilk::data::uint64,lyramilk::data::string> rainbow_table;

		lyramilk::cave::leveldb_standard* dbins;
	  public:
		static void static_init_dispatch()
		{
			redislike_session::static_init_dispatch();
			// def_cmd(命令,参数最少数量(如果是变长参数则为负数),标记,第一个key参数的序号,最后一个key参数的序号,重复参数的步长);
			#define def_cmd(cmd,ac,fg,fk,lk,kc)  regist_command(#cmd,(redis_cmd_callback)&leveldb_standard_redislike_session::notify_##cmd,ac,fg,fk,lk,kc)
			def_cmd(hscan,3,redis_cmd_spec::readonly|redis_cmd_spec::fast|redis_cmd_spec::noscript,0,0,0);
			def_cmd(hlen,2,redis_cmd_spec::readonly|redis_cmd_spec::slow|redis_cmd_spec::noscript,1,1,1);
			def_cmd(info,1,redis_cmd_spec::readonly|redis_cmd_spec::skip_monitor|redis_cmd_spec::fast|redis_cmd_spec::noscript,0,0,0);
			def_cmd(scan,2,redis_cmd_spec::readonly|redis_cmd_spec::fast|redis_cmd_spec::noscript,0,0,0);
			def_cmd(type,2,redis_cmd_spec::readonly|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);
			def_cmd(get,2,redis_cmd_spec::readonly|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);

			def_cmd(spop,2,redis_cmd_spec::write|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);
			def_cmd(sscan,3,redis_cmd_spec::readonly|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);
			def_cmd(scard,2,redis_cmd_spec::readonly|redis_cmd_spec::slow|redis_cmd_spec::noscript,1,1,1);

			def_cmd(zscan,3,redis_cmd_spec::readonly|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);
			def_cmd(zrange,-4,redis_cmd_spec::readonly|redis_cmd_spec::noscript,1,1,1);
			def_cmd(zcard,2,redis_cmd_spec::readonly|redis_cmd_spec::slow|redis_cmd_spec::noscript,1,1,1);

			def_cmd(del,2,redis_cmd_spec::write|redis_cmd_spec::fast|redis_cmd_spec::noscript,1,1,1);

			def_cmd(subscribe,2,redis_cmd_spec::readonly|redis_cmd_spec::fast|redis_cmd_spec::skip_monitor|redis_cmd_spec::pubsub|redis_cmd_spec::noscript,0,0,0);
			def_cmd(publish,3,redis_cmd_spec::write|redis_cmd_spec::fast|redis_cmd_spec::skip_monitor|redis_cmd_spec::pubsub|redis_cmd_spec::noscript,0,0,0);
			def_cmd(compact,1,redis_cmd_spec::write|redis_cmd_spec::noscript,0,0,0);

			#undef def_cmd
		}
		virtual void init_cavedb(const lyramilk::data::string& masterid,const lyramilk::data::string& requirepass,lyramilk::cave::leveldb_standard* dbins,bool readonly);

		virtual result_status notify_hscan(const lyramilk::data::array& cmd, std::ostream& os);
		virtual result_status notify_hlen(const lyramilk::data::array& cmd, std::ostream& os);

		virtual result_status notify_sscan(const lyramilk::data::array& cmd, std::ostream& os);
		virtual result_status notify_scard(const lyramilk::data::array& cmd, std::ostream& os);
		virtual result_status notify_spop(const lyramilk::data::array& cmd, std::ostream& os);

		virtual result_status notify_zscan(const lyramilk::data::array& cmd, std::ostream& os);
		virtual result_status notify_zcard(const lyramilk::data::array& cmd, std::ostream& os);
		virtual result_status notify_zrange(const lyramilk::data::array& cmd, std::ostream& os);

		virtual result_status notify_get(const lyramilk::data::array& cmd, std::ostream& os);

		virtual result_status notify_subscribe(const lyramilk::data::array& cmd, std::ostream& os);
		virtual result_status notify_publish(const lyramilk::data::array& cmd, std::ostream& os);
		virtual result_status notify_compact(const lyramilk::data::array& cmd, std::ostream& os);
		virtual result_status notify_info(const lyramilk::data::array& cmd, std::ostream& os);
		virtual result_status notify_scan(const lyramilk::data::array& cmd, std::ostream& os);
		virtual result_status notify_type(const lyramilk::data::array& cmd, std::ostream& os);
	};

}}









#endif
