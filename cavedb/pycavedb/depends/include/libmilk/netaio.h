#ifndef _lyramilk_netio_netaio_h_
#define _lyramilk_netio_netaio_h_

#include "netio.h"
#include "aio.h"

namespace lyramilk{namespace netio
{
	/*
	using lyramilk::io::native_epool_type;
	using lyramilk::io::native_filedescriptor_type;
	using lyramilk::io::uint32;
*/
	/**
		@brief 套接字会话
		@details 被aiolistener监听到以后可以通过创建这个对象的实例创建通信会话。。
	*/
	class _lyramilk_api_ aiosession : public lyramilk::io::aioselector,public lyramilk::netio::socket
	{
	  protected:
		int flag;
		lyramilk::data::string peer_cert_info;
		aiosession();
		virtual ~aiosession();
		virtual void ondestory();
	  public:
		virtual bool init() = 0;
		/**
			@brief 销毁会话，如果重写的话，也要在重写的函数中调用 aiosession::destory 来触发ondestory以便释放对象。
		*/
		virtual void destory();

		/**
			@brief 获取对端证书信息。
		*/
		virtual lyramilk::data::string ssl_get_peer_certificate_info();

		virtual lyramilk::io::native_filedescriptor_type getfd();
	  private:
		typedef aiosession* (*builder)();
		typedef void(*destoryer)(aiosession* s);
		friend class aiolistener;

		friend class aiosession_sync;
		friend class aiosession_async;
	  public:
		/// 模板化会话对象的销毁函数
		template <typename T>
		static void __tdestoryer(aiosession* s)
		{
			delete (T*)(s);
		}

		destoryer dtr;
		/// 模板化会话对象的生成函数
		template <typename T>
		static T* __tbuilder()
		{
			T* p = new T();
			p->dtr = __tdestoryer<T>;
			return p;
		}
	};

	/**
		@brief 同步套接字会话
		@details onrequest中向os写数据的时候，以阻塞的方式写出去。
	*/
	class _lyramilk_api_ aiosession_sync : public lyramilk::netio::aiosession
	{
	  protected:
		socket_ostream aos;
		virtual bool notify_in();
		virtual bool notify_out();
		virtual bool notify_hup();
		virtual bool notify_err();
		virtual bool notify_pri();
	  public:
		aiosession_sync();
		virtual ~aiosession_sync();

		virtual bool init();
		virtual void destory();
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
			@return 返回false会导致服务器主动断开链接。
		*/
		virtual bool onrequest(const char* cache, int size, lyramilk::data::ostream& os) = 0;
	};

	/**
		@brief 异步套接字会话
		@details onrequest中向os写数据的时候，先被缓存起来，得到epoll的OUT消息的时候会把数据写出去。
	*/
	class _lyramilk_api_ aiosession_async : public lyramilk::netio::aiosession_sync
	{
		lyramilk::data::stringstream scache;
		lyramilk::data::string retransmitcache;
	  protected:
		virtual bool notify_in();
		virtual bool notify_out();
		virtual int cache_read(char* buf,int bufsize);
		virtual bool cache_empty();
		virtual bool cache_ok();
		virtual void cache_clear();
	  public:
		aiosession_async();
		virtual ~aiosession_async();
	};


	/**
		@brief 异步套接字监听会话
	*/
	class _lyramilk_api_ aiolistener : public lyramilk::io::aioselector,public socket
	{
		ssl_ctx_type sslctx;
		bool use_ssl;
		bool ssl_self_adaptive;
		virtual bool notify_in();
		virtual bool notify_out();
		virtual bool notify_hup();
		virtual bool notify_err();
		virtual bool notify_pri();
	public:
		aiolistener();
		virtual ~aiolistener();

		/**
			@brief 打开一个端口并监听。
			@param port 被打开的端口。
			@return 如果打开成功返回true。
		*/
		virtual bool open(lyramilk::data::uint16 port);

		/**
			@brief 打开一个端口并监听。
			@param host 绑定的ip地址。
			@param port 被打开的端口。
			@return 如果打开成功返回true。
		*/
		virtual bool open(const lyramilk::data::string& host,lyramilk::data::uint16 port);

		/**
			@brief 创建一个会话。
			@return 创建的会话。这个会话应该用destory释放。
		*/
		virtual lyramilk::netio::aiosession* create() = 0;

		/**
			@brief 开启客户端验证(双向认证)
			@param clientca 客户端ca证书
			@param force 是否强制认证。
			@return false表示失败
		*/
		virtual bool ssl_use_client_verify(bool force = false);

		/**
			@brief 加载可信证书
			@param verify_locations 客户端ca证书
			@return false表示失败
		*/
		virtual bool ssl_load_verify_locations(const lyramilk::data::array& verify_locations);

		/**
			@brief 初始化SSL并自动开启SSL。
			@param certfilename 证书
			@param keyfilename 私钥
			@param verify_locations 信任链
			@return false表示失败
		*/
		virtual bool init_ssl(const lyramilk::data::string& certfilename, const lyramilk::data::string& keyfilename);

		/**
			@brief 开启或关闭SSL。
			@param use_ssl 为false表示关闭SSL，新创建的会话将以明文传输。
			@param ssl_self_adaptive 当开启SSL时对客户端自适应。为true时，当客户端以非SSL进行连接时，服务器以非SSL接受。为false时，客户端以非SSL进行连接时，强制关闭该会话。
		*/
		virtual void ssl(bool use_ssl, bool ssl_self_adaptive = false);

		/// 检查该套接字的通信是否己加密。
		virtual bool ssl();

		///	取得SSL_CTX*
		virtual ssl_ctx_type get_ssl_ctx();

		virtual lyramilk::io::native_filedescriptor_type getfd();

		virtual void ondestory();
	};

	/**
	@brief 异步套接字服务器
	*/
	template <typename T>
	class aioserver : public aiolistener
	{
		virtual lyramilk::netio::aiosession* create()
		{
			return lyramilk::netio::aiosession::__tbuilder<T>();
		}
	};
}}

#endif
