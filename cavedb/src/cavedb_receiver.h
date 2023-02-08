#ifndef _cavedb_cavedb_receiver_h_
#define _cavedb_cavedb_receiver_h_

#include <libmilk/thread.h>
#include <libmilk/netio.h>
#include "cmd_accepter.h"
#include "resp.h"

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{

	class cavedb_receiver:public lyramilk::threading::threads
	{
		lyramilk::netio::client c;
		lyramilk::netio::socket_istream is;
		lyramilk::data::string host;
		lyramilk::data::uint16 port;
		lyramilk::data::string masterauth;
		lyramilk::data::string masterid;

		lyramilk::data::string psync_replid;
		lyramilk::data::uint64 psync_offset;
		cmd_accepter* cmdr;
		cmdsessiondata sen;

		enum st_status{
			st_running,
			st_sync,
			st_idle,
			st_stop,
		}status;

	  protected:
		bool reconnect();
		bool sync_once(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::uint64 count,lyramilk::data::array* cmds,lyramilk::data::string* nextreplid,lyramilk::data::uint64* nextseq);
		virtual int svc();
	  public:
		bool readonly;


		cavedb_receiver();
		virtual ~cavedb_receiver();

		void init(const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& pwd,const lyramilk::data::string& masterid,lyramilk::data::string psync_replid,lyramilk::data::uint64 psync_offset,cmd_accepter* cmdr);
		lyramilk::data::uint64 tell_offset();
		virtual st_status get_sync_status();
	};


}}
#endif
