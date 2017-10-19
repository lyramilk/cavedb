#ifndef _casedb_casedb_h_
#define _casedb_casedb_h_

#include <leveldb/db.h>
#include <libmilk/var.h>
#include <libmilk/log.h>
#include <libmilk/iterator.h>
#include "slave.h"
#include "leveldb_util.h"

namespace lyramilk{ namespace cave
{
	typedef bool (*kv_call_back)(const lyramilk::data::string& key,leveldb::Slice& field,leveldb::Slice& value,void* userdata);
	typedef bool (*hashmap_call_back)(const lyramilk::data::string& key,leveldb::Slice& field,leveldb::Slice& value,void* userdata);
	typedef bool (*list_call_back)(const lyramilk::data::string& key,lyramilk::data::int64 index,leveldb::Slice& value,void* userdata);
	typedef bool (*zset_call_back)(const lyramilk::data::string& key,double score,leveldb::Slice& value,void* userdata);
	typedef bool (*set_call_back)(const lyramilk::data::string& key,leveldb::Slice& value,void* userdata);

	class redis_leveldb_handler;
	class database:public lyramilk::cave::slave
	{
		leveldb::DB* ldb;
		leveldb::WriteOptions wopt;
		redis_leveldb_handler* redis_cmd_args;
		time_t lastcompat;
		lyramilk::log::logss log;
		lyramilk::threading::threads *h;
	  public:
		database();
		virtual ~database();

		bool init_leveldb(const lyramilk::data::string& leveldbpath,const leveldb::Options& opts);

		bool slaveof_redis(const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& pwd);
		bool slaveof_ssdb(const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& pwd);

		bool slaveof_redis(const lyramilk::data::string& leveldbpath,const leveldb::Options& opts,const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& pwd);
		bool slaveof_ssdb(const lyramilk::data::string& leveldbpath,const leveldb::Options& opts,const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& pwd);

		void dump(std::ostream& os);
		//key
		virtual lyramilk::data::int64 ttl(lyramilk::data::uint64 dbid,const lyramilk::data::string& key);
		virtual lyramilk::data::int64 pttl(lyramilk::data::uint64 dbid,const lyramilk::data::string& key);
		//virtual bool scan(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,kv_call_back cbk,void* userdata);
		//string
		virtual lyramilk::data::string get(lyramilk::data::uint64 dbid,const lyramilk::data::string& key);
		//hashmap
		virtual bool hexists(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& field);
		virtual lyramilk::data::string hget(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& field);
		virtual lyramilk::data::var::map hgetall(lyramilk::data::uint64 dbid,const lyramilk::data::string& key);
		virtual lyramilk::data::uint64 hlen(lyramilk::data::uint64 dbid,const lyramilk::data::string& key);
		virtual bool hscan(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,hashmap_call_back cbk,void* userdata);
		//set
		virtual lyramilk::data::uint64 scard(lyramilk::data::uint64 dbid,const lyramilk::data::string& key);
		virtual bool sdiff(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& key2,set_call_back cbk,void* userdata);
		virtual bool sinter(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& key2,set_call_back cbk,void* userdata);
		virtual bool sunion(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& key2,set_call_back cbk,void* userdata);
		virtual bool sismember(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& value);
		virtual bool smembers(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,set_call_back cbk,void* userdata);
		//virtual bool srandmember(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,lyramilk::data::string* outstryy );
		virtual bool sscan(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,set_call_back cbk,void* userdata);
		//zset
		virtual lyramilk::data::uint64 zcard(lyramilk::data::uint64 dbid,const lyramilk::data::string& key);
		virtual lyramilk::data::uint64 zcount(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,double min,double max);
		virtual bool zrange(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,lyramilk::data::int64 start,lyramilk::data::int64 stop,zset_call_back cbk,void* userdata);
		virtual bool zrangebyscore(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,double min,double max,zset_call_back cbk,void* userdata);
		virtual lyramilk::data::uint64 zrank(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& value);
		virtual bool zrevrange(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,lyramilk::data::int64 start,lyramilk::data::int64 stop,zset_call_back cbk,void* userdata);
		virtual bool zrevrangebyscore(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,double min,double max,zset_call_back cbk,void* userdata);
		virtual lyramilk::data::uint64 zrevrank(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& value);
		virtual double zscore(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,const lyramilk::data::string& value);
		virtual bool zscan(lyramilk::data::uint64 dbid,const lyramilk::data::string& key,zset_call_back cbk,void* userdata);
		//control
	  protected:
		virtual void notify_command(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args);
		virtual bool notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset);
		virtual bool notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset);
	};

	class siterator: public lyramilk::data::bidirectional_iterator<siterator,leveldb::Slice>
	{
		database* db;
		lyramilk::data::string key;
	  protected:
		virtual leveldb::Slice& get();
		virtual void next();
		virtual void prev();
		virtual bool equal(const siterator& c) const;
		virtual siterator& assign(const siterator &o);
	  public:
		siterator(database* db,const lyramilk::data::string& key);
		virtual ~siterator();
	};

	class ziterator: public lyramilk::data::bidirectional_iterator<ziterator,std::pair<double,leveldb::Slice> >
	{
		database* db;
		lyramilk::data::string key;
	  protected:
		virtual std::pair<double,leveldb::Slice>& get();
		virtual void next();
		virtual void prev();
		virtual bool equal(const ziterator& c) const;
		virtual ziterator& assign(const ziterator &o);
	  public:
		ziterator(database* db,const lyramilk::data::string& key);
		virtual ~ziterator();
	};

	class hiterator: public lyramilk::data::bidirectional_iterator<hiterator,std::pair<double,leveldb::Slice> >
	{
		database* db;
		lyramilk::data::string key;
	  protected:
		virtual std::pair<double,leveldb::Slice>& get();
		virtual void next();
		virtual void prev();
		virtual bool equal(const hiterator& c) const;
		virtual hiterator& assign(const hiterator &o);
	  public:
		hiterator(database* db,const lyramilk::data::string& key);
		virtual ~hiterator();
	};

}}

#endif
