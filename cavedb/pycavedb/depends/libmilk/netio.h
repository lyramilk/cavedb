#ifndef _lyramilk_system_netio_h_
#define _lyramilk_system_netio_h_

#include <memory>
#include <exception>

#include "var.h"
#include "thread.h"

#ifndef SSL_TYPE
	typedef void* SSL_TYPE;
	typedef void* SSL_CTX_TYPE;
#endif

/**
	@namespace lyramilk::netio
	@brief 网络io
	@details 该命名空间描述网络io通信
*/
namespace lyramilk{namespace netio
{
#ifdef WIN32
	typedef SOCKET native_socket_type;
#elif defined __linux__
	typedef int native_socket_type;
#endif
	typedef SSL_TYPE ssl_type;
	typedef SSL_CTX_TYPE ssl_ctx_type;

	class _lyramilk_api_ netaddress
	{
	  public:
		///网络字节序
		lyramilk::data::string host;
		///本地字节序
		lyramilk::data::uint16 port;

		netaddress(const lyramilk::data::string& host, lyramilk::data::uint16 port);
		netaddress(lyramilk::data::uint32 ipv4, lyramilk::data::uint16 port);
		netaddress(lyramilk::data::uint16 port);
		netaddress(const lyramilk::data::string& hostandport);
		netaddress();
		lyramilk::data::string ip_str() const;
	};

	class _lyramilk_api_ socket
	{
		native_socket_type sock;
	  protected:
		friend class socket_ostream_buf;
		friend class socket_istream_buf;
		ssl_type sslobj;
		bool sslenable;
	  public:
		socket();
		virtual ~socket();
		//检查该套接字的通信是否己加密
		virtual bool ssl();

		///	取得SSL*
		virtual ssl_type get_ssl_obj();

		//判断套接字是否可用
		virtual bool isalive();

		//关闭套接字
		virtual bool close();

		/// 取得本端ip
		virtual netaddress source() const;

		/// 取得对端ip
		virtual netaddress dest() const;

		/// 取得套接字
		virtual native_socket_type fd() const;
		virtual void fd(native_socket_type tmpfd);

		/*
			@brief 从套接字中读取数据
		*/

		virtual lyramilk::data::int32 read(void* buf, lyramilk::data::int32 len);

		/*
			@brief 从套接字中预览数据
		*/

		virtual lyramilk::data::int32 peek(void* buf, lyramilk::data::int32 len);

		/*
			@brief 向套接字写入数据
		*/
		virtual lyramilk::data::int32 write(const void* buf, lyramilk::data::int32 len);

		/*
			@brief 检查套接字是否可读
			@param msec 等待的毫秒数。
			@return 在 msec 毫秒内如果套接字可读则返回true。
		*/
		virtual bool check_read(lyramilk::data::uint32 msec);

		/*
			@brief 检查套接字是否可写
			@param msec 等待的毫秒数。
			@return 在 msec 毫秒内如果套接字可写则返回true。
		*/
		virtual bool check_write(lyramilk::data::uint32 msec);

		/*
			@brief 检查套接字是有错误
		*/
		virtual bool check_error();

		static bool check_read(native_socket_type fd,lyramilk::data::uint32 msec);
		static bool check_write(native_socket_type fd,lyramilk::data::uint32 msec);
		static bool check_error(native_socket_type fd);
	};

	/// 以流的方式操作套接字的流缓冲
	class _lyramilk_api_ socket_ostream_buf : public std::basic_streambuf<char>
	{
		friend class socket_ostream;
		lyramilk::data::uint64 seq_w;
	  protected:
		lyramilk::netio::socket* psock;
		char* putbuf;
		virtual int_type sync();
		virtual int_type overflow (int_type c = traits_type::eof());
		virtual std::streamsize xsputn (const char* s, std::streamsize n);
	  public:
		void reset();
		socket_ostream_buf();
		virtual ~socket_ostream_buf();
	};

	/*
		@brief 以流的方式操作套接字的流
		@details 只支持写
	*/
	class _lyramilk_api_ socket_ostream : public lyramilk::data::ostringstream
	{
		socket_ostream_buf sbuf;
		int flags;
	  public:
		socket_ostream();
		socket_ostream(socket* ac);
		virtual ~socket_ostream();
		void init(socket* ac);
		lyramilk::data::uint64 wseq();
	};



	/// 以流的方式操作套接字的流缓冲（异步）
	class _lyramilk_api_ socket_ostream_buf_async : public std::basic_streambuf<char>
	{
		friend class socket_ostream_async;
		lyramilk::data::uint64 seq_w;
		int seq_diff;
	  protected:
		lyramilk::netio::socket* psock;
		virtual int_type overflow (int_type c = traits_type::eof());
		virtual std::streamsize xsputn (const char* s, std::streamsize n);
	  public:
		void reset();
		socket_ostream_buf_async();
		virtual ~socket_ostream_buf_async();
	};

	/*
		@brief 以流的方式操作套接字的流（异步）
		@details 只支持写，不能保证一次把所有数据都写出去，需要用pcount去读到底写了多少字节。
	*/
	class _lyramilk_api_ socket_ostream_async : public lyramilk::data::ostringstream
	{
		socket_ostream_buf_async sbuf;
		int flags;
	  public:
		socket_ostream_async();
		socket_ostream_async(socket* ac);
		virtual ~socket_ostream_async();
		void init(socket* ac);
		lyramilk::data::uint64 wseq();
		int pcount();
	};

	/// 以流的方式操作套接字的流缓冲
	class _lyramilk_api_ socket_istream_buf : public std::basic_streambuf<char>
	{
		friend class socket_istream;
		lyramilk::data::uint64 seq_r;
	  protected:
		lyramilk::netio::socket* psock;
		char* getbuf;
		virtual int_type sync();
		virtual int_type underflow();
	  public:
		void reset();
		socket_istream_buf();
		virtual ~socket_istream_buf();
	};

	/*
		@brief 以流的方式操作套接字的流
		@details 只支持读
	*/
	class _lyramilk_api_ socket_istream : public lyramilk::data::istringstream
	{
		socket_istream_buf sbuf;
		int flags;
	  public:
		socket_istream();
		socket_istream(socket* ac);
		virtual ~socket_istream();
		void init(socket* ac);
		virtual std::streamsize in_avail();
		lyramilk::data::uint64 rseq();
	};

	/// 客户端套接字
	class _lyramilk_api_ client : public socket
	{
		ssl_ctx_type sslctx;
		bool use_ssl;
	public:
		client();
		virtual ~client();

		virtual bool open(const netaddress& addr);
		virtual bool open(const lyramilk::data::string& host,lyramilk::data::uint16 port);

		virtual bool ssl();
		virtual void ssl(bool use_ssl);
		virtual bool init_ssl(const lyramilk::data::string& certfilename = "", const lyramilk::data::string& keyfilename = "");

		///	取得SSL_CTX*
		virtual ssl_ctx_type get_ssl_ctx();
	};


}}

#endif
