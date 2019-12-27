#include "netio.h"
#include "dict.h"
#include "log.h"
#include <sys/epoll.h>
#include <sys/poll.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <cassert>

#include <iostream>
#include <streambuf>

#ifdef OPENSSL_FOUND
	#include <openssl/bio.h>
	#include <openssl/crypto.h>
	#include <openssl/evp.h>
	#include <openssl/x509.h>
	#include <openssl/x509v3.h>
	#include <openssl/ssl.h>
	#include <openssl/err.h>
	#include <string.h>
#endif

#ifdef OPENSSL_FOUND
	struct __ssl
	{
		__ssl()
		{
			SSL_library_init();
			SSL_load_error_strings();
			ERR_load_BIO_strings();
			OpenSSL_add_all_algorithms();
			gethostbyname("127.0.0.1");
		}
		~__ssl()
		{
		}

		lyramilk::data::string err()
		{
			char buff[4096] = {0};
			ERR_error_string(ERR_get_error(),buff);
			return buff;
		}
	};
#else

	struct __ssl
	{
		__ssl()
		{
			gethostbyname("127.0.0.1");
		}
		~__ssl()
		{
		}

		lyramilk::data::string err()
		{
			return "";
		}
	};
#endif
static __ssl _ssl;



namespace lyramilk{namespace netio
{
	/* netaddress */
	netaddress::netaddress(const lyramilk::data::string& host, lyramilk::data::uint16 port)
	{
		this->host = host;
		this->port = port;

	}

	netaddress::netaddress(lyramilk::data::uint32 ipv4, lyramilk::data::uint16 port)
	{
		in_addr in;
		in.s_addr = ipv4;
		host = inet_ntoa(in);
		this->port = port;
	}

	netaddress::netaddress(lyramilk::data::uint16 port)
	{
		this->port = port;
	}

	netaddress::netaddress(const lyramilk::data::string& hostandport)
	{
		std::size_t sz = hostandport.find(':');
		if(sz == hostandport.npos){
			host = hostandport;
			port = 0;
		}else{
			host = hostandport.substr(0,sz);
			lyramilk::data::var vport = hostandport.substr(sz+1);
			port = vport;
		}

	}

	netaddress::netaddress()
	{
		this->port = 0;
	}

	lyramilk::data::string netaddress::ip_str() const
	{
		return this->host;
	}

	/* socket */
	socket::socket()
	{
		sslobj = nullptr;
		sslenable = false;
		sock = -1;
	}

	socket::~socket()
	{
		close();
#ifdef OPENSSL_FOUND
		if(sslobj){
			SSL_shutdown((SSL*)sslobj);
			SSL_free((SSL*)sslobj);
		}
#endif
	}

	bool socket::ssl()
	{
		return sslobj != nullptr;
	}

	ssl_type socket::get_ssl_obj()
	{
		return sslobj;
	}

	bool socket::isalive()
	{
		if(fd() < 0) return false;
		pollfd pfd;
		pfd.fd = fd();
		pfd.events = POLLOUT;
		pfd.revents = 0;
		int ret = ::poll(&pfd,1,0);
		if(ret > 0){
			if(pfd.revents & (POLLERR | POLLHUP | POLLRDHUP)){
				return false;
			}
			if(pfd.revents & POLLOUT){
				return true;
			}
		}
		return false;
	}

	bool socket::close()
	{
		if(sock >= 0){
			::close(sock);
			sock = -1;
			return true;
		}
		return false;
	}

	netaddress socket::source() const
	{
		sockaddr_in addr;
		socklen_t size = sizeof addr;
		if(getsockname(fd(),(sockaddr*)&addr,&size) !=0 ) return netaddress();
		return netaddress(addr.sin_addr.s_addr,ntohs(addr.sin_port));
	}

	netaddress socket::dest() const
	{
		sockaddr_in addr;
		socklen_t size = sizeof addr;
		if(getpeername(fd(),(sockaddr*)&addr,&size) !=0 ) return netaddress();
		return netaddress(addr.sin_addr.s_addr,ntohs(addr.sin_port));
	}

	native_socket_type socket::fd() const
	{
		return sock;
	}

	void socket::fd(native_socket_type tmpfd)
	{
		sock = tmpfd;
	}

	lyramilk::data::int32 socket::read(void* buf, lyramilk::data::int32 len)
	{
		int rt = 0;
#ifdef OPENSSL_FOUND
		if(ssl()){
			rt = SSL_read((SSL*)get_ssl_obj(), buf, len);
		}else{
			rt = ::recv(fd(),buf,len,0);
		}
#else
		rt = ::recv(fd(),buf,len,0);
#endif
		if(rt < 0){
			if(errno != EAGAIN){
				lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.socket.read") << lyramilk::kdict("从套接字%d中读取数据时发生错误:%s",fd(),strerror(errno)) << std::endl;
			}
		}else if(rt == 0){
			//lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.socket.read") << lyramilk::kdict("从套接字%d中读取数据时发生错误:%s",fd(),"套接字己关闭") << std::endl;
		}
		return rt;
	}


	lyramilk::data::int32 socket::peek(void* buf, lyramilk::data::int32 len)
	{
		int rt = 0;
#ifdef OPENSSL_FOUND
		if(ssl()){
			rt = SSL_peek((SSL*)get_ssl_obj(), buf, len);
		}else{
			rt = ::recv(fd(),buf,len,MSG_PEEK);
		}
#else
		rt = ::recv(fd(),buf,len,MSG_PEEK);
#endif
		if(rt < 0){
			if(errno != EAGAIN){
				lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.socket.peek") << lyramilk::kdict("从套接字%d中预览数据时发生错误:%s",fd(),strerror(errno)) << std::endl;
			}
		}else if(rt == 0){
			//lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.socket.peek") << lyramilk::kdict("从套接字%d中预览数据时发生错误:%s",fd(),"套接字己关闭") << std::endl;
		}
		return rt;
	}


	lyramilk::data::int32 socket::write(const void* buf, lyramilk::data::int32 len)
	{
		int rt = 0;
#ifdef OPENSSL_FOUND
		if(ssl()){
			rt = SSL_write((SSL*)get_ssl_obj(), buf, len);
		}else{
			rt = ::send(fd(),buf,len,0);
		}
#else
		rt = ::send(fd(),buf,len,0);
#endif

		if(rt < 0){
			if(errno != EAGAIN){
				lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.socket.write") << lyramilk::kdict("向套接字%d中写入数据时发生错误:%s",fd(),strerror(errno)) << std::endl;
			}
		}else if(rt == 0){
			//lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.socket.write") << lyramilk::kdict("向套接字%d中写入数据时发生错误:%s",fd(),"套接字己关闭") << std::endl;
		}
		return rt;
	}


	bool socket::check_read(lyramilk::data::uint32 msec)
	{
		pollfd pfd;
		pfd.fd = fd();
		pfd.events = POLLIN;
		pfd.revents = 0;
		int ret = ::poll(&pfd,1,msec);
		if(ret > 0){
			if(pfd.revents & POLLIN){
				return true;
			}
		}
		return false;
	}

	bool socket::check_write(lyramilk::data::uint32 msec)
	{
		pollfd pfd;
		pfd.fd = fd();
		pfd.events = POLLOUT;
		pfd.revents = 0;
		int ret = ::poll(&pfd,1,msec);
		if(ret > 0){
			if(pfd.revents & POLLOUT){
				return true;
			}
		}
		return false;
	}

	bool socket::check_error()
	{
		pollfd pfd;
		pfd.fd = fd();
		pfd.events = POLLHUP | POLLERR;
		pfd.revents = 0;
		int ret = ::poll(&pfd,1,0);
		if(ret > 0){
			if(pfd.revents){
				return true;
			}
		}
		return false;
	}

	bool socket::check_read(native_socket_type fd,lyramilk::data::uint32 msec)
	{
		pollfd pfd;
		pfd.fd = fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		int ret = ::poll(&pfd,1,msec);
		if(ret > 0){
			if(pfd.revents & POLLIN){
				return true;
			}
		}
		return false;
	}

	bool socket::check_write(native_socket_type fd,lyramilk::data::uint32 msec)
	{
		pollfd pfd;
		pfd.fd = fd;
		pfd.events = POLLOUT;
		pfd.revents = 0;
		int ret = ::poll(&pfd,1,msec);
		if(ret > 0){
			if(pfd.revents & POLLOUT){
				return true;
			}
		}
		return false;
	}

	bool socket::check_error(native_socket_type fd)
	{
		pollfd pfd;
		pfd.fd = fd;
		pfd.events = POLLHUP | POLLERR;
		pfd.revents = 0;
		int ret = ::poll(&pfd,1,0);
		if(ret > 0){
			if(pfd.revents){
				return true;
			}
		}
		return false;
	}


	/* socket_ostream_buf */

	const int buffersize = 2048;

	socket_ostream_buf::int_type socket_ostream_buf::sync()
	{
		if(putbuf){
			const char* p = pbase();
			int n = pptr() - p;
			if(n == 0) return 0;

			int sendbytes = 0;
			while(sendbytes < n){
				int r = psock->write(p + sendbytes,n - sendbytes);
				if(r > 0){
					sendbytes += r;
					seq_w += r;
				}else if(r < 0 && errno== EAGAIN){
					usleep(10);
					continue;
				}else{
					lyramilk::klog(lyramilk::log::error,"lyramilk.socket_ostream_buf.sync") << lyramilk::kdict("发送：%d\t错误：%s",r,strerror(errno)) << std::endl;
					return traits_type::eof();
				}
			}

			if(sendbytes == n){
				setp(putbuf,putbuf + buffersize);
			}else{
				char buff[buffersize];
				memcpy(buff,putbuf + sendbytes,n - sendbytes);
				setp(putbuf,putbuf + buffersize);
				sputn(buff,n-sendbytes);
			}
			return 0;
		}
		return traits_type::eof();
	}

	socket_ostream_buf::int_type socket_ostream_buf::overflow (int_type c)
	{
		if(putbuf == nullptr){
			char t = c;
			int r = psock->write(&t,1);
			if(r > 0){
				seq_w += r;
				return r;
			}
		}
		if(sync() == 0){
			if(c != traits_type::eof()){
				sputc(c);
			}
			return c;
		}
		return traits_type::eof();
	}

	std::streamsize socket_ostream_buf::xsputn (const char* s, std::streamsize n)
	{
		if(putbuf){
			return std::basic_streambuf<char>::xsputn(s,n);
		}

		std::streamsize sendbytes = 0;

		while(sendbytes < n){
			int r = psock->write(s + sendbytes,n - sendbytes);
			if(r > 0){
				sendbytes += r;
				seq_w += r;
			}else if(r < 0 && errno== EAGAIN){
				usleep(10);
				continue;
			}else{
				break;
			}
		}
		return sendbytes;
	}

	socket_ostream_buf::socket_ostream_buf()
	{
		putbuf = nullptr;
	}

	socket_ostream_buf::~socket_ostream_buf()
	{
		if(putbuf) delete[] putbuf;
	}

	void socket_ostream_buf::reset()
	{
		if(putbuf){
			setp(putbuf,putbuf + buffersize);
		}
	}

	/* socket_ostream */
	socket_ostream::socket_ostream()
	{
		sbuf.psock = nullptr;
	}

	socket_ostream::socket_ostream(socket* ac)
	{
		init(ac);
	}

	socket_ostream::~socket_ostream()
	{
	}

	void socket_ostream::init(socket* ac)
	{
		sbuf.seq_w = 0;
		sbuf.psock = ac;
		lyramilk::data::ostringstream::init(&sbuf);
		sbuf.putbuf = new char[buffersize];
		sbuf.reset();
		clear();
	}

	lyramilk::data::uint64 socket_ostream::wseq()
	{
		return sbuf.seq_w;
	}



	/* socket_ostream_buf_async */
	socket_ostream_buf_async::int_type socket_ostream_buf_async::overflow (int_type c)
	{
		char t = c;
		if(psock->write(&t,1) == 1){
			++seq_w;
			seq_diff = 1;
			return t;
		}
		return traits_type::eof();
	}

	std::streamsize socket_ostream_buf_async::xsputn (const char* s, std::streamsize n)
	{
		seq_diff = psock->write(s,n);
		if(seq_diff > 0){
			seq_w += seq_diff;
			return seq_diff;
		}
		return traits_type::eof();
	}

	socket_ostream_buf_async::socket_ostream_buf_async()
	{
	}

	socket_ostream_buf_async::~socket_ostream_buf_async()
	{
	}

	/* socket_ostream_async */
	socket_ostream_async::socket_ostream_async()
	{
		sbuf.psock = nullptr;
	}

	socket_ostream_async::socket_ostream_async(socket* ac)
	{
		init(ac);
	}

	socket_ostream_async::~socket_ostream_async()
	{
	}

	void socket_ostream_async::init(socket* ac)
	{
		sbuf.seq_w = 0;
		sbuf.seq_diff = 0;
		sbuf.psock = ac;
		lyramilk::data::ostringstream::init(&sbuf);
		clear();
	}

	lyramilk::data::uint64 socket_ostream_async::wseq()
	{
		return sbuf.seq_w;
	}

	int socket_ostream_async::pcount()
	{
		return sbuf.seq_diff;
	}

	/* socket_istream_buf */
	socket_istream_buf::int_type socket_istream_buf::sync()
	{
		if(getbuf == nullptr) return traits_type::eof();
		return overflow();
	}

	socket_istream_buf::int_type socket_istream_buf::underflow()
	{
		if(getbuf){
			char* p = getbuf;
			int l = buffersize;
			int r = 0;
			do{
				r = psock->read(p,l);
				if(errno == EAGAIN){
					usleep(10);
				}
			}while(r < 0 && errno == EAGAIN);
			if(r>0){
				seq_r += r;
				setg(getbuf,getbuf,getbuf + r);
				return sgetc();
			}
			lyramilk::klog(lyramilk::log::error,"lyramilk.socket_istream_buf.underflow") << lyramilk::kdict("接收：%d\t错误：%s",r,strerror(errno)) << std::endl;
			return traits_type::eof();
		}
		char c;
		int r = psock->read(&c,1);
		if(r > 0){
			seq_r += r;
			return c;
		}
		return traits_type::eof();
	}

	socket_istream_buf::socket_istream_buf()
	{
		getbuf = nullptr;
	}

	socket_istream_buf::~socket_istream_buf()
	{
		if(getbuf) delete[] getbuf;
	}

	void socket_istream_buf::reset()
	{
		if(getbuf){
			setg(getbuf,getbuf + buffersize,getbuf + buffersize);
		}
	}

	/* socket_istream */
	socket_istream::socket_istream()
	{
		sbuf.psock = nullptr;
	}

	socket_istream::socket_istream(socket* ac)
	{
		init(ac);
	}

	socket_istream::~socket_istream()
	{
	}

	void socket_istream::init(socket* ac)
	{
		sbuf.seq_r = 0;
		sbuf.psock = ac;
		lyramilk::data::istringstream::init(&sbuf);
		sbuf.getbuf = new char[buffersize];
		sbuf.reset();
		clear();
	}

	std::streamsize socket_istream::in_avail()
	{
		return sbuf.in_avail();
	}

	lyramilk::data::uint64 socket_istream::rseq()
	{
		return sbuf.seq_r - in_avail();
	}

	/* client */
	client::client():use_ssl(false)
	{
		sslctx = nullptr;
	}

	client::~client()
	{
#ifdef OPENSSL_FOUND
		if(sslctx){
			SSL_CTX_free((SSL_CTX*)sslctx);
		}
#endif
	}

	bool client::open(const netaddress& addr)
	{
		return open(addr.ip_str(),addr.port);
	}

	bool client::open(const lyramilk::data::string& host,lyramilk::data::uint16 port)
	{
		if(fd() >= 0){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.client.open") << lyramilk::kdict("打开套接字(%s:%u)失败：%s",host.c_str(),port,"套接字己经打开。") << std::endl;
			return false;
		}
		hostent* h = gethostbyname(host.c_str());
		if(h == nullptr){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.client.open") << lyramilk::kdict("打开套接字(%s:%u)失败：%s",host.c_str(),port,strerror(errno)) << std::endl;
			return false;
		}

		in_addr* inaddr = (in_addr*)h->h_addr;
		if(inaddr == nullptr){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.client.open") << lyramilk::kdict("打开套接字(%s:%u)失败：%s",host.c_str(),port,strerror(errno)) << std::endl;
			return false;
		}

		native_socket_type tmpsock = ::socket(AF_INET,SOCK_STREAM, IPPROTO_IP);
		if(tmpsock < 0){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.client.open") << lyramilk::kdict("打开套接字(%s:%u)失败：%s",host.c_str(),port,strerror(errno)) << std::endl;
			return false;
		}

		sockaddr_in addr = {0};
		addr.sin_addr.s_addr = inaddr->s_addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);


		if(0 == ::connect(tmpsock,(const sockaddr*)&addr,sizeof(addr))){
#ifdef OPENSSL_FOUND
			if(use_ssl && sslctx){
				SSL* sslptr = SSL_new((SSL_CTX*)sslctx);
				if(SSL_set_fd(sslptr,tmpsock) != 1) {
					sslptr = nullptr;
					lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.client.ssl.onevent") << lyramilk::kdict("绑定套接字(%s:%u)失败:%s",host.c_str(),port,_ssl.err().c_str()) << std::endl;
					::close(tmpsock);
					return false;
				}

				SSL_set_connect_state(sslptr);
				if(SSL_do_handshake(sslptr) != 1) {
					lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.client.ssl.onevent") << lyramilk::kdict("握手(%s:%u)失败:%s",host.c_str(),port,_ssl.err().c_str()) << std::endl;
					::close(tmpsock);
					return false;
				}
				this->sslobj = sslptr;
			}
#endif
			/*
			unsigned int argp = 1;
			//ioctlsocket(tmpsock,FIONBIO,&argp);
			ioctl(tmpsock,FIONBIO,&argp);*/
			this->fd(tmpsock);
			return true;
		}
		lyramilk::klog(lyramilk::log::error,"lyramilk.netio.client.open") << lyramilk::kdict("打开套接字(%s:%u)失败：%s",host.c_str(),port,strerror(errno)) << std::endl;
		::close(tmpsock);
		return false;
	}

	bool client::ssl()
	{
		return socket::ssl();
	}

	void client::ssl(bool use_ssl)
	{
		this->use_ssl = use_ssl;
	}

	bool client::init_ssl(const lyramilk::data::string& certfilename, const lyramilk::data::string& keyfilename)
	{
#ifdef OPENSSL_FOUND
		if(sslctx == nullptr){
			sslctx = SSL_CTX_new(SSLv23_client_method());
		}
		int r = 0;
		if(!certfilename.empty()){
			r = SSL_CTX_use_certificate_file((SSL_CTX*)sslctx, certfilename.c_str(), SSL_FILETYPE_PEM);
			if(r != 1) {
				lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.client.ssl.init_ssl") << lyramilk::kdict("设置公钥失败:%s",_ssl.err().c_str()) << std::endl;
				return false;
			}
		}
		if(!keyfilename.empty()){
			r = SSL_CTX_use_PrivateKey_file((SSL_CTX*)sslctx, keyfilename.c_str(), SSL_FILETYPE_PEM);
			if(r != 1) {
				lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.client.ssl.init_ssl") << lyramilk::kdict("设置私钥失败:%s",_ssl.err().c_str()) << std::endl;
				return false;
			}
		}
		if(!certfilename.empty() && !keyfilename.empty()){
			r = SSL_CTX_check_private_key((SSL_CTX*)sslctx);
			if(r != 1) {
				lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.client.ssl.init_ssl") << lyramilk::kdict("验证公钥失败:%s",_ssl.err().c_str()) << std::endl;
				return false;
			}
		}
		SSL_CTX_set_options((SSL_CTX*)sslctx, SSL_OP_TLS_ROLLBACK_BUG);
		ssl(true);
		return true;
#else
		lyramilk::klog(lyramilk::log::error,"lyramilk.netio.client.ssl.init_ssl") << lyramilk::kdict("不支持SSL") << std::endl;
		return false;
#endif
	}

	ssl_ctx_type client::get_ssl_ctx()
	{
#ifdef OPENSSL_FOUND
		if(sslctx == nullptr){
			sslctx = SSL_CTX_new(SSLv23_client_method());
		}
#endif
		return sslctx;
	}

}}

