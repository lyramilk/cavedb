#ifndef _cavedb_slave_h_redis_like_session_h_
#define _cavedb_slave_h_redis_like_session_h_

//#include "store/leveldb_standard.h"
//#include "slave_ssdb.h"
#include "redis_session.h"
#include "store.h"
#include "store_reader.h"


namespace lyramilk{ namespace cave
{

	class redislike_session;

	typedef lyramilk::cave::redis_session::result_status (redislike_session::*redis_cmd_callback)(const lyramilk::data::array& cmd, std::ostream& os);

	struct redis_cmd_spec
	{
		const static int write = 0x1;// - command may result in modifications
		const static int readonly = 0x2;// - command will never modify keys
		const static int denyoom = 0x4;// - reject command if currently OOM
		const static int admin = 0x8;// - server admin command
		const static int pubsub = 0x10;// - pubsub-related command
		const static int noscript =0x20;// - deny this command from scripts
		const static int random = 0x40;// - command has random results, dangerous for scripts
		const static int sort_for_script = 0x80;// - if called from script, sort output
		const static int loading = 0x100;// - allow command while database is loading
		const static int stale = 0x200;// - allow command while replica has stale data
		const static int skip_monitor = 0x400;// - do not show this command in MONITOR
		const static int asking = 0x800;// - cluster related - accept even if importing
		const static int fast = 0x1000;// - command operates in constant or log(N) time. Used for latency monitoring.
		const static int noauth = 0x2000;// 非redis状态，而是cavedb特有的，允许在非登录状态下可用这个，该命令不在command命令中返回。

		redis_cmd_callback c;

		int n;
		int f;
		int firstkey;
		int lastkey;
		int keystepcount;
	};

	class redislike_session:public lyramilk::netio::aiosession_sync,public lyramilk::cave::redis_session
	{
	  private:
		static std::map<lyramilk::data::string,redis_cmd_spec> dispatch;
	  protected:
		bool readonly;
		lyramilk::data::string requirepass;
		lyramilk::data::string pass;
		lyramilk::data::string masterid;

		lyramilk::cave::store_reader* reader;
		lyramilk::cave::store* store;

		static void inline regist_command(const lyramilk::data::string& cmd,redis_cmd_callback callback,int argcount,int flag,int firstkey_offset,int lastkey_offset,int keystepcount)
		{
			redislike_session::dispatch[cmd].c = callback;
			redislike_session::dispatch[cmd].f = flag;
			redislike_session::dispatch[cmd].firstkey = firstkey_offset;
			redislike_session::dispatch[cmd].lastkey = lastkey_offset;
			redislike_session::dispatch[cmd].keystepcount = keystepcount;
			redislike_session::dispatch[cmd].n = argcount;
		}
		static void static_init_dispatch();
	  public:
		redislike_session();
		virtual ~redislike_session();

		virtual void init_cavedb(const lyramilk::data::string& masterid,const lyramilk::data::string& requirepass,lyramilk::cave::store* store,lyramilk::cave::store_reader* reader,bool readonly);

		virtual bool oninit(lyramilk::data::ostream& os);
		virtual bool onrequest(const char* cache, int size, lyramilk::data::ostream& os);

		lyramilk::cave::redis_session::result_status notify_cmd(const lyramilk::data::array& cmd, void* userdata);

		lyramilk::cave::redis_session::result_status notify_auth(const lyramilk::data::array& cmd, std::ostream& os);
		lyramilk::cave::redis_session::result_status notify_cavedb_sync(const lyramilk::data::array& cmd, std::ostream& os);
		lyramilk::cave::redis_session::result_status notify_command(const lyramilk::data::array& cmd, std::ostream& os);

		lyramilk::cave::redis_session::result_status notify_del(const lyramilk::data::array& cmd, std::ostream& os);

		lyramilk::cave::redis_session::result_status notify_ping(const lyramilk::data::array& cmd, std::ostream& os);
		lyramilk::cave::redis_session::result_status notify_monitor(const lyramilk::data::array& cmd, std::ostream& os);
		lyramilk::cave::redis_session::result_status notify_sample(const lyramilk::data::array& cmd, std::ostream& os)
		{
			return rs_ok;
		}

		lyramilk::cave::redis_session::result_status notify_hgetall(const lyramilk::data::array& cmd, std::ostream& os);
		lyramilk::cave::redis_session::result_status notify_hget(const lyramilk::data::array& cmd, std::ostream& os);
		lyramilk::cave::redis_session::result_status notify_hexist(const lyramilk::data::array& cmd, std::ostream& os);
		lyramilk::cave::redis_session::result_status notify_hset(const lyramilk::data::array& cmd, std::ostream& os);
		lyramilk::cave::redis_session::result_status notify_hmset(const lyramilk::data::array& cmd, std::ostream& os);
		lyramilk::cave::redis_session::result_status notify_hdel(const lyramilk::data::array& cmd, std::ostream& os);

		lyramilk::cave::redis_session::result_status notify_sadd(const lyramilk::data::array& cmd, std::ostream& os);
		lyramilk::cave::redis_session::result_status notify_srem(const lyramilk::data::array& cmd, std::ostream& os);

		lyramilk::cave::redis_session::result_status notify_zadd(const lyramilk::data::array& cmd, std::ostream& os);
		lyramilk::cave::redis_session::result_status notify_zrem(const lyramilk::data::array& cmd, std::ostream& os);

		lyramilk::cave::redis_session::result_status notify_set(const lyramilk::data::array& cmd, std::ostream& os);
	};

}}
#endif
