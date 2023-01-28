#ifndef _cavedb_ssdb_receiver_h_
#define _cavedb_ssdb_receiver_h_

#include <libmilk/thread.h>
#include <libmilk/netio.h>
#include "cmd_accepter.h"

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class ssdb_receiver:public lyramilk::threading::threads
	{
		lyramilk::netio::client c;
		lyramilk::netio::socket_istream is;
		unsigned int loadsum;
		lyramilk::data::string host;
		lyramilk::data::uint16 port;
		lyramilk::data::string pwd;
		lyramilk::data::string masterid;
		cmd_accepter* cmdr;
		cmdsessiondata sen;
	  public:
		lyramilk::data::string requirepass;
		bool readonly;
		enum st_status{
			st_running,
			st_sync,
			st_idle,
			st_stop,
		}status;
	  protected:
		bool reconnect();
		bool exec(const lyramilk::data::array& cmd,lyramilk::data::strings* ret);
		bool push(const lyramilk::data::array& cmd);
		bool pop(lyramilk::data::strings* ret);
	  public:
		ssdb_receiver();
		virtual ~ssdb_receiver();

		void init(const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& pwd,const lyramilk::data::string& masterid,lyramilk::data::string psync_replid,lyramilk::data::uint64 psync_offset,cmd_accepter* cmdr);
		lyramilk::data::uint64 tell_offset();

		lyramilk::data::string static hexmem(const void *p, int size);
		virtual int svc();

		virtual st_status get_sync_status();
	  protected:
		lyramilk::data::string psync_replid;
		lyramilk::data::uint64 psync_offset;
	  protected:
		virtual bool proc_noop(lyramilk::data::uint64 seq);
		virtual bool proc_copy(lyramilk::data::uint64 seq,char cmd,const char* p,std::size_t l,const lyramilk::data::strings& args);
		virtual bool proc_sync(lyramilk::data::uint64 seq,char cmd,const char* p,std::size_t l,const lyramilk::data::strings& args);
	};


}}
#endif
