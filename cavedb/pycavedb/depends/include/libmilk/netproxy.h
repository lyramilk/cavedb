#ifndef _lyramilk_netio_netproxy_h_
#define _lyramilk_netio_netproxy_h_

#include "netaio.h"
#include "netio.h"
#include "aio.h"
#include <arpa/inet.h>

/**
	@namespace lyramilk::netio
*/

namespace lyramilk{namespace netio
{
	class _lyramilk_api_ aioproxysession_speedy : public lyramilk::netio::aiosession
	{
		friend class aioproxysession;
		ssl_ctx_type sslctx;
		bool use_ssl;
	  public:
		aioproxysession_speedy* endpoint;
	  public:
		aioproxysession_speedy();
	  	virtual ~aioproxysession_speedy();
		virtual bool init();
		virtual bool combine(aioproxysession_speedy* endpoint);

		/**
			@brief 连接上游服务
			@param timeout_msec 毫秒超时间，设置为-2则立即成功。
			@return true成功 false失败。
		*/
		virtual bool open(const lyramilk::data::string& host,lyramilk::data::uint16 port,int timeout_msec);
		/**
			@brief 连接上游服务
			@param timeout_msec 毫秒超时间，设置为-2则立即成功。
			@return true成功 false失败。
		*/
		virtual bool open(const sockaddr_in& saddr,int timeout_msec);

		virtual bool ssl();
		virtual void ssl(bool use_ssl);
		virtual bool init_ssl(const lyramilk::data::string& certfilename = "", const lyramilk::data::string& keyfilename = "");

		///	取得SSL_CTX*
		virtual ssl_ctx_type get_ssl_ctx();
	  protected:
		virtual bool notify_in();
		virtual bool notify_out();
		virtual bool notify_hup();
		virtual bool notify_err();
		virtual bool notify_pri();
	};



	/**
		@brief 同步套接字会话
		@details onrequest中向os写数据的时候，以阻塞的方式写出去。
	*/
	class _lyramilk_api_ aioproxysession : public aioproxysession_speedy
	{
	  protected:
		bool directmode;
		socket_ostream aos;
		virtual bool notify_in();
		virtual bool notify_out();
		virtual bool notify_hup();
		virtual bool notify_err();
		virtual bool notify_pri();
		virtual void ondestory();
	  public:
		aioproxysession();
		virtual ~aioproxysession();

		virtual bool init();

		virtual bool combine(const lyramilk::data::string& host,lyramilk::data::uint16 port);
		virtual bool combine(const sockaddr_in& saddr);
		virtual bool combine(aioproxysession_speedy* dest);
		virtual bool start_proxy();
		virtual bool stop_proxy();
	  protected:
		/**
			@brief 连接时触发
			@return 返回false会导致服务器主动断开链接。
		*/
		virtual bool oninit(lyramilk::data::ostream& os);

		/**
			@brief 断开时触发
			@return 返回false会导致服务器主动断开链接。
		*/
		virtual void onfinally(lyramilk::data::ostream& os);

		/**
			@brief 收到数据时触发。
			@param cache 这里面有新数据。
			@param size 新数据的字节数。
			@param size 在代理会话中实际使用的字节数，如果小于size的话，cache中末尾的数据还会继续触发事件。
			@return 返回false会导致服务器主动断开链接。
		*/
		virtual bool onrequest(const char* cache, int size, int* sizeused, lyramilk::data::ostream& os) = 0;
	};

}}

#endif
