#ifndef _cavedb_leveldb_store_h_
#define _cavedb_leveldb_store_h_

#include <libmilk/var.h>
#include "cmd_accepter.h"
#include "binlog_store.h"

namespace leveldb{class DB;};

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{

	class speed_counter
	{
		volatile lyramilk::data::uint64 speed_count;
		lyramilk::data::uint64 speed;
		time_t speed_tm;
	  public:
		speed_counter();
		~speed_counter();
		lyramilk::data::uint64 operator ++();
		operator lyramilk::data::uint64();
	};



	class leveldb_store:public cmd_accepter
	{
		leveldb::DB* ldb;
		cmdstatus on_hgetall(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen) const;
		cmdstatus on_hget(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen) const;
		cmdstatus on_hmget(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen) const;
		cmdstatus on_hexist(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen) const;
		cmdstatus on_hset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen) const;
		cmdstatus on_hmset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen) const;
		cmdstatus on_hdel(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen) const;

		static void* thread_auto_compact(leveldb::DB* ldb);
		binlog* blog;
	  public:
		speed_counter rspeed;
		speed_counter wspeed;
	  public:
		leveldb_store();
		virtual ~leveldb_store();

		bool open_leveldb(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing);
		void set_binlog(binlog* blog);

		virtual bool check_command(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen,const command_sepc& cmdspec);
		virtual void after_command(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdsessiondata* sen,const command_sepc& cmdspec,cmdstatus retcs);


	};





}}

#endif

