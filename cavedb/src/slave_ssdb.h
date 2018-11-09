#ifndef _casedb_slave_ssdb_h_
#define _casedb_slave_ssdb_h_

#include <libmilk/thread.h>
#include <libmilk/netio.h>
#include "slave.h"

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class slave_ssdb:public lyramilk::threading::threads
	{
		lyramilk::netio::client c;
		lyramilk::netio::socket_istream is;
		unsigned int loadsum;
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
		bool exec(const lyramilk::data::array& cmd,lyramilk::data::strings* ret);
		bool push(const lyramilk::data::array& cmd);
		bool pop(lyramilk::data::strings* ret);
	  public:
		slave_ssdb();
		virtual ~slave_ssdb();
		void slaveof(const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& pwd,lyramilk::data::string psync_replid,lyramilk::data::uint64 psync_offset,slave* peventhandler);
		lyramilk::data::uint64 tell_offset();

		lyramilk::data::string static hexmem(const void *p, int size);
	  protected:
		virtual int svc();
		lyramilk::data::string psync_replid;
		lyramilk::data::uint64 psync_offset;
	  protected:
		virtual void proc_noop(lyramilk::data::uint64 seq);
		virtual void proc_copy(lyramilk::data::uint64 seq,char cmd,const char* p,std::size_t l,const lyramilk::data::strings& args);
		virtual void proc_sync(lyramilk::data::uint64 seq,char cmd,const char* p,std::size_t l,const lyramilk::data::strings& args);
	};

}}

#endif
