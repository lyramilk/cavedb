#include "null_store.h"

namespace lyramilk{ namespace cave
{
	null_store::null_store()
	{
	}

	null_store::~null_store()
	{
	}

	bool null_store::notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
	{
		return true;
	}

	void null_store::notify_flushdb(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
	}

	void null_store::notify_flushall(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
	}

	void null_store::notify_del(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
	}

	void null_store::notify_move(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
	}

	void null_store::notify_pexpireat(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
	}

	void null_store::notify_persist(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
	}

	void null_store::notify_rename(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
	}


	void null_store::notify_hset(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
	}

	void null_store::notify_hdel(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
	{
	}

	bool null_store::get_sync_info(lyramilk::data::string* replid,lyramilk::data::uint64* offset) const
	{
		return false;
	}

	bool null_store::hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		rspeed_on_read();
		return false;
	}

	lyramilk::data::string null_store::hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const
	{
		rspeed_on_read();
		return "";
	}

	lyramilk::data::stringdict null_store::hgetall(const lyramilk::data::string& key) const
	{
		rspeed_on_read();
		lyramilk::data::stringdict m;
		return m;
	}
}}
