#ifndef _cavedb_store_reader_h_
#define _cavedb_store_reader_h_

#include <libmilk/var.h>
#include <libmilk/thread.h>

/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{
	class store_reader
	{
		mutable lyramilk::data::uint64 rspeed_counter;
		mutable lyramilk::data::uint64 rspeed_speed;
		mutable time_t rspeed_tm;
	  protected:
		void rspeed_on_read() const;
	  public:
		store_reader();
		virtual ~store_reader();
			/// 获取读取速度
		virtual lyramilk::data::uint64 rspeed() const;
			/// 获取同步进度
		virtual bool get_sync_info(const lyramilk::data::string& masterid,lyramilk::data::string* replid,lyramilk::data::uint64* offset) const = 0;
			///	[hashmap]	判断key是否存在
		virtual bool hexist(const lyramilk::data::string& key,const lyramilk::data::string& field) const = 0;
			///	[hashmap]	根据key和field取得一个value
		virtual lyramilk::data::string hget(const lyramilk::data::string& key,const lyramilk::data::string& field) const = 0;
			///	[hashmap]	取得一个key的所有value
		virtual lyramilk::data::stringdict hgetall(const lyramilk::data::string& key) const = 0;
			///	[hashmap]	取得一个key的所有value，以var::map形似返回
		virtual lyramilk::data::map hgetallv(const lyramilk::data::string& key) const;
	};
}}

#endif
