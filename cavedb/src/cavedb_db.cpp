#include "cavedb.h"
#include "redis_to_leveldb.h"
#include <libmilk/multilanguage.h>
#include <libmilk/log.h>
#include <libmilk/testing.h>

namespace lyramilk{ namespace cave
{
	static lyramilk::log::logss log("lyramilk.cave.cavedb");

	void static redis_flushdb(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		leveldb::ReadOptions ropt;
		leveldb::WriteOptions wopt;

		leveldb_iterator it(rh.ldb->NewIterator(ropt));
		std::string prefix = rh.dbprefix(-1);

		if(it) for(it->Seek(prefix);it->Valid();it->Next()){
			if(!it->key().starts_with(prefix)) break;
			rh.ldb->Delete(wopt,it->key());
		}
		rh.ldb->CompactRange(nullptr,nullptr);
	}

	void static redis_flushall(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		leveldb::ReadOptions ropt;
		leveldb::WriteOptions wopt;

		leveldb_iterator it(rh.ldb->NewIterator(ropt));
		if(it) for(it->SeekToFirst();it->Valid();it->Next()){
			rh.ldb->Delete(wopt,it->key());
		}
		rh.ldb->CompactRange(nullptr,nullptr);
		rh.ldb->Put(wopt,".cfver",redis_leveldb_handler::cfver);
		batch.Clear();
	}

	void static redis_ping(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
	}

	void static redis_select(redis_leveldb_handler& rh,leveldb::WriteBatch &batch,lyramilk::data::var::array& args)
	{
		rh.select(args[1]);
	}

	static __attribute__ ((constructor)) void __init()
	{
		cavedb_redis_commands::instance()->define("flushdb",redis_flushdb);
		cavedb_redis_commands::instance()->define("flushall",redis_flushall);
		cavedb_redis_commands::instance()->define("select",redis_select);
		cavedb_redis_commands::instance()->define("ping",redis_ping);
	}

}}
