#ifndef _cavedb_leveldb_store_h_
#define _cavedb_leveldb_store_h_

#include <libmilk/var.h>
#include "cmd_accepter.h"
#include "cavedb_key.h"

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



	class leveldb_store:protected cmd_accepter
	{
		leveldb::DB* ldb;
		cmdstatus on_hgetall(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_hget(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_hmget(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_hexist(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_hset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_hdel(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_hscan(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;

		cmdstatus on_sadd(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_srem(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_smembers(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_sscan(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;

		cmdstatus on_del(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_cave_sync(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		
		cmdstatus on_scan(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_type(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;

/*
		cmdstatus on_sadd(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_srem(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_sprepop(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
		cmdstatus on_spop(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen) const;
*/

		static void* thread_auto_compact(leveldb::DB* ldb);

		time_t last_time;
	  public:
		speed_counter rspeed;
		speed_counter wspeed;
	  public:
		leveldb_store();
		virtual ~leveldb_store();

		bool open_leveldb(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing);

		virtual bool save_sync_info(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset) const;
		virtual bool get_sync_info(const lyramilk::data::string& masterid,lyramilk::data::string* replid,lyramilk::data::uint64* offset) const;

		virtual bool check_command(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen,const command_sepc& cmdspec);
		//virtual void after_command(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,const lyramilk::data::array& args,lyramilk::data::var* ret,cmdchanneldata* chd,cmdsessiondata* sen,const command_sepc& cmdspec,cmdstatus retcs);

		cmdstatus hgetall(const lyramilk::data::string& key,lyramilk::data::stringdict* ret) const;
		cmdstatus hget(const lyramilk::data::string& key,const lyramilk::data::string& field,lyramilk::data::string* ret) const;
		cmdstatus hscan(const lyramilk::data::string& key,const lyramilk::data::string& cursor,lyramilk::data::uint64 count,lyramilk::data::string* nextcursor,lyramilk::data::strings* result) const;

		cmdstatus smembers(const lyramilk::data::string& key,lyramilk::data::strings* ret) const;
		cmdstatus sscan(const lyramilk::data::string& key,const lyramilk::data::string& cursor,lyramilk::data::uint64 count,lyramilk::data::string* nextcursor,lyramilk::data::strings* result) const;

		
		cmdstatus scan(const lyramilk::data::string& cursor,lyramilk::data::uint64 count,lyramilk::data::string* nextcursor,lyramilk::data::strings* result) const;
		cmdstatus type(const lyramilk::data::string& key,cavedb_key_type* result) const;

	};





}}

#endif

