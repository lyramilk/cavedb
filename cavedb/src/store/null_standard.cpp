#include "null_standard.h"
#include <libmilk/log.h>
#include <libmilk/dict.h>
#include <libmilk/testing.h>
#include <libmilk/codes.h>
#include <libmilk/hash.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

namespace lyramilk{ namespace cave
{
	lyramilk::log::logss static log(lyramilk::klog,"lyramilk.cave.store.null_standard");

	bool null_standard::notify_idle(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata)
	{
		return true;
	}

	bool null_standard::notify_psync(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,void* userdata)
	{
		return true;
	}

	bool null_standard::notify_flushdb(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return true;
	}

	bool null_standard::notify_flushall(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return true;
	}

	bool null_standard::notify_del(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return true;
	}

	bool null_standard::notify_move(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		log(lyramilk::log::error,__FUNCTION__) << D("未实现%s函数，这在ssdb中不应该出现","move") << std::endl;
		return false;
	}

	bool null_standard::notify_pexpireat(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return false;
	}

	bool null_standard::notify_persist(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return false;
	}

	bool null_standard::notify_rename(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		log(lyramilk::log::error,__FUNCTION__) << D("未实现%s函数，这在ssdb中不应该出现","rename") << std::endl;
		return false;
	}

	bool null_standard::notify_hset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return true;
	}

	bool null_standard::notify_hmset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return true;
	}

	bool null_standard::notify_hdel(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return true;
	}


	bool null_standard::notify_set(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return true;
	}

	bool null_standard::notify_ssdb_del(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return true;
	}

	bool null_standard::notify_ssdb_qset(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return true;
	}

	bool null_standard::notify_lpop(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		log(lyramilk::log::error,__FUNCTION__) << D("未实现%s函数","lpop") << std::endl;
		return false;
	}

	bool null_standard::notify_rpop(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		log(lyramilk::log::error,__FUNCTION__) << D("未实现%s函数","rpop") << std::endl;
		return false;
	}

	bool null_standard::notify_zadd(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return true;
	}

	bool null_standard::notify_zrem(const lyramilk::data::string& masterid,const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args,void* userdata)
	{
		return true;
	}

	null_standard::null_standard()
	{
	}

	null_standard::~null_standard()
	{
	}

	bool null_standard::get_sync_info(const lyramilk::data::string& masterid,lyramilk::data::string* replid,lyramilk::data::uint64* offset) const
	{
		if(replid == nullptr || offset == nullptr) return false;
		*replid = "";
		*offset = 0;
		return true;
	}

	bool null_standard::compact()
	{
		return true;
	}


	bool null_standard::zrange(const lyramilk::data::string& key,lyramilk::data::int64 start,lyramilk::data::int64 stop,bool withscore,lyramilk::data::strings* result) const
	{
		if(result)result->clear();
		return true;
	}

	lyramilk::data::string null_standard::zscan(const lyramilk::data::string& key,const lyramilk::data::string& current,lyramilk::data::uint64 count,lyramilk::data::strings* result) const
	{
		if(result)result->clear();
		return "0";
	}


	lyramilk::data::uint64 null_standard::zcard(const lyramilk::data::string& key) const
	{
		return 0;
	}

	bool null_standard::get(const lyramilk::data::string& key,lyramilk::data::string* value) const
	{
		if(value)value->clear();
		return true;
	}

	bool null_standard::hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		return false;
	}

	lyramilk::data::string null_standard::hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		return "";
	}

	bool null_standard::hget(const lyramilk::data::string& key,const lyramilk::data::string& field,lyramilk::data::string* value) const
	{
		if(value)value->clear();
		return false;
	}

	lyramilk::data::stringdict null_standard::hgetall(const lyramilk::data::string& key) const
	{
		lyramilk::data::stringdict result;
		return result;
	}


	lyramilk::data::string null_standard::scan(const lyramilk::data::string& current,lyramilk::data::uint64 count,lyramilk::data::strings* result) const
	{
		if(result)result->clear();
		return "";
	}

	lyramilk::data::string null_standard::hscan(const lyramilk::data::string& key,const lyramilk::data::string& current,lyramilk::data::uint64 count,lyramilk::data::strings* result) const
	{
		if(result)result->clear();
		return "";
	}

	lyramilk::data::uint64 null_standard::hlen(const lyramilk::data::string& key) const
	{
		return 0;
	}

	lyramilk::data::string null_standard::type(const lyramilk::data::string& key) const
	{
		return "";
	}

	bool null_standard::is_on_full_sync()
	{
		return on_full_sync();
	}

	lyramilk::data::string null_standard::get_property(const lyramilk::data::string& property)
	{
		return "";
	}

	bool null_standard::open_leveldb(const lyramilk::data::string& leveldbpath,unsigned int cache_size_MB,bool create_if_missing)
	{
		return true;
	}

}}
