#ifndef _casedb_slave_redis_h_
#define _casedb_slave_redis_h_

#include <libmilk/thread.h>
#include <libmilk/netio.h>
#include "slave.h"

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class slave_redis:public lyramilk::threading::threads
	{
		lyramilk::netio::client c;
		lyramilk::netio::socket_stream is;
		lyramilk::data::uint64 psync_rseq_diff;
		unsigned int loadsum;
		double loadcoff;
		unsigned int loadalive;
		enum {
			st_running,
			st_sync,
			st_idle,
			st_stop,
		}status;
		lyramilk::data::string host;
		lyramilk::data::uint16 port;
		lyramilk::data::string pwd;
		slave* peventhandler;
	  protected:
		bool reconnect();
		bool exec(const lyramilk::data::var::array& cmd,lyramilk::data::var* ret);
		bool push(const lyramilk::data::var::array& cmd);
		bool pop(lyramilk::data::var* ret);
	  public:
		slave_redis();
		virtual ~slave_redis();
		void slaveof(const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& pwd,lyramilk::data::string psync_replid,lyramilk::data::uint64 psync_offset,slave* peventhandler);
		lyramilk::data::uint64 tell_offset();
	  protected:
		virtual int svc();
		lyramilk::data::string psync_replid;
		lyramilk::data::uint64 psync_offset;
	};
}}

#endif
