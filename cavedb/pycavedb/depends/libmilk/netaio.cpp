#include "netaio.h"
#include "dict.h"
#include "log.h"

#include <sys/epoll.h>
#include <sys/poll.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <cassert>
#include <netdb.h>

#ifdef OPENSSL_FOUND
	#include <openssl/ssl.h>
	#include <openssl/err.h>
#endif


#ifdef OPENSSL_FOUND
lyramilk::data::string inline ssl_err()
{
	char buff[4096] = {0};
	ERR_error_string(ERR_get_error(),buff);
	return buff;
}
#endif

namespace lyramilk{namespace netio
{
	// aiosession
	aiosession::aiosession()
	{
		flag = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLONESHOT;
	}

	aiosession::~aiosession()
	{
	}

	void aiosession::ondestory()
	{
		dtr(this);
	}

	void aiosession::destory()
	{
		if(pool){
			pool->remove(this);
		}
		ondestory();
	}


	lyramilk::io::native_filedescriptor_type aiosession::getfd()
	{
		return socket::fd();
	}

	lyramilk::data::string aiosession::ssl_get_peer_certificate_info()
	{
		return peer_cert_info;
	}

	// aiosession_sync
	aiosession_sync::aiosession_sync()
	{
	}

	aiosession_sync::~aiosession_sync()
	{
	}

	bool aiosession_sync::notify_in()
	{
		char buff[4096];
		int i = 0;
		do{
			i = read(buff,sizeof(buff));
			if(i == 0) return false;
			if(i < 0){
				if(errno == EAGAIN ) break;
				if(errno == EINTR) continue;
				return false;
			}
			assert(i > 0);

			if(!onrequest(buff,i,aos)){
				aos.flush();
				return false;
			}
			aos.flush();
		}while(i > 0);

		return pool->reset(this,flag);
	}

	bool aiosession_sync::notify_out()
	{
		lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aiosession_sync.notify_out") << lyramilk::kdict("%s方法未实现","notify_out") << std::endl;
		return false;
	}

	bool aiosession_sync::notify_hup()
	{
		lyramilk::klog(lyramilk::log::debug,"lyramilk.netio.aiosession_sync.notify_hup") << lyramilk::kdict("发生了HUP事件%s",strerror(errno)) << std::endl;
		return false;
	}

	bool aiosession_sync::notify_err()
	{
		lyramilk::klog(lyramilk::log::debug,"lyramilk.netio.aiosession_sync.notify_err") << lyramilk::kdict("发生了ERR事件%s",strerror(errno)) << std::endl;
		return false;
	}

	bool aiosession_sync::notify_pri()
	{
		lyramilk::klog(lyramilk::log::debug,"lyramilk.netio.aiosession_sync.notify_pri") << lyramilk::kdict("发生了PRI事件%s",strerror(errno)) << std::endl;
		return false;
	}

	bool aiosession_sync::init()
	{
		aos.init(this);
		return oninit(aos);
	}

	void aiosession_sync::destory()
	{
		onfinally(aos);
		aiosession::destory();
	}
	
	bool aiosession_sync::oninit(lyramilk::data::ostream& os)
	{
		return true;
	}

	void aiosession_sync::onfinally(lyramilk::data::ostream& os)
	{
	}

	/******** aiosession_async ************/
	aiosession_async::aiosession_async()
	{}

	aiosession_async::~aiosession_async()
	{}

	bool aiosession_async::notify_in()
	{
		char buff[4096];
		int i = 0;
		if(cache_ok()){
			cache_clear();
		}
		do{
			i = read(buff,sizeof(buff));

			if(i == 0) return false;
			if(i < 0){
				if(errno == EAGAIN ) break;
				if(errno == EINTR) continue;
				return false;
			}
			assert(i > 0);

			if(!onrequest(buff,i,scache)){
				flag &= ~EPOLLIN;
				return notify_out();
			}
		}while(i > 0);

		return notify_out();
	}

	bool aiosession_async::notify_out()
	{
		char buff[4096];
		while(cache_ok()){
			int sendcount = 0;
			if(retransmitcache.empty()){
				sendcount = cache_read(buff,4096);
			}else{
				sendcount = retransmitcache.size();
				assert(sendcount < 4096);
				retransmitcache.copy(buff,sendcount);
			}
			if(sendcount == 0){
				break;
			}
			assert(sendcount > 0 && sendcount <= 4096);
			int rt = write(buff,sendcount);

			if(rt == 0) return false;
			if(rt < 0){
				if(errno == EAGAIN || errno == EINTR){
					cache_clear();
					retransmitcache.assign(buff,sendcount);
					break;
				}
				return false;
			}
			assert(rt > 0);
			if(rt < sendcount){
				cache_clear();
				retransmitcache.assign(buff + rt,sendcount - rt);
				break;
			}
			retransmitcache.clear();
		}
		if(!cache_empty()){
			return pool->reset(this,EPOLLOUT | flag);
		}else{
			cache_clear();
		}

		if(flag & EPOLLIN){
			return pool->reset(this,flag);
		}
		return false;
	}

	int aiosession_async::cache_read(char* buff,int bufsize)
	{
		scache.read(buff,4096);
		return scache.gcount();
	}

	bool aiosession_async::cache_empty()
	{
		if(scache){
			return scache.rdbuf()->in_avail() == 0;
		}
		return true;
	}

	bool aiosession_async::cache_ok()
	{
		return scache.good();
	}

	void aiosession_async::cache_clear()
	{
		scache.str("");
		scache.clear();
	}

	// aiolistener
#ifdef OPENSSL_FOUND
	static bool sock_read_able(native_socket_type sock,int delay)
	{
		pollfd pfd;
		pfd.fd = sock;
		pfd.events = POLLIN;
		pfd.revents = 0;
		int ret = ::poll(&pfd,1,delay);
		if(ret > 0){
			if(pfd.revents & POLLIN){
				return true;
			}
		}
		return false;
	}
#endif

	bool aiolistener::notify_in()
	{
		sockaddr_in addr;
		socklen_t addr_size = sizeof(addr);
		native_socket_type acceptfd = ::accept(fd(),(sockaddr*)&addr,&addr_size);
		if(acceptfd < 0) return true;
#ifdef OPENSSL_FOUND
		SSL* sslptr = nullptr;
		if(use_ssl && sslctx){
			struct timeval timeout;
			timeout.tv_sec = 3;
			timeout.tv_usec = 0;
			setsockopt(acceptfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
			setsockopt(acceptfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

			sslptr = SSL_new((SSL_CTX*)sslctx);
			if(SSL_set_fd(sslptr,acceptfd) != 1) {
				SSL_shutdown(sslptr);
				SSL_free(sslptr);
				sslptr = nullptr;
				if(!ssl_self_adaptive){
					lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.aiolistener.EPOLLIN.ssl") << lyramilk::kdict("绑定套接字失败:%s",ssl_err().c_str()) << std::endl;
					::close(acceptfd);
					return true;
				}else{
					lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.aiolistener.EPOLLIN.ssl") << lyramilk::kdict("在自适应模式中绑定套接字失败:%s",ssl_err().c_str()) << std::endl;
				}
			}

			if(ssl_self_adaptive){
				if(sslptr){
					SSL_set_accept_state(sslptr);
					if(sock_read_able(acceptfd,600)){
						char buff[6];
						if(::recv(acceptfd,buff,6,MSG_PEEK)>0){
							if(buff[0] == 0x16 && buff[5] == 0x01){
								if(SSL_do_handshake(sslptr) != 1) {
									lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.aiolistener.EPOLLIN.ssl.handshake") << lyramilk::kdict("在自适应模式中握手失败:%s",ssl_err().c_str()) << std::endl;
									SSL_shutdown(sslptr);
									SSL_free(sslptr);
									::close(acceptfd);
									return true;
								}
							}else{
								SSL_shutdown(sslptr);
								SSL_free(sslptr);
								sslptr = nullptr;
							}
						}
					}else{
						lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.aiolistener.EPOLLIN.ssl.handshake") << lyramilk::kdict("在自适应模式中握手失败:%s","握手超时。") << std::endl;
						SSL_shutdown(sslptr);
						SSL_free(sslptr);
						sslptr = nullptr;
					}
				}
			}else{
				SSL_set_accept_state(sslptr);
				if(SSL_do_handshake(sslptr) != 1) {
					lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.aiolistener.EPOLLIN.ssl.handshake") << lyramilk::kdict("握手失败:%s",ssl_err().c_str()) << std::endl;
					SSL_shutdown(sslptr);
					SSL_free(sslptr);
					::close(acceptfd);
					return true;
				}
			}
		}
#endif
		aiosession* s = create();
		if(s == nullptr){
			lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.aiolistener.EPOLLIN") << lyramilk::kdict("创建会话失败") << std::endl;
			::close(acceptfd);
			return true;
		}
		s->fd(acceptfd);
#ifdef OPENSSL_FOUND
		s->sslobj = sslptr;
		X509* x = SSL_get_peer_certificate(sslptr);
		if(x && X509_V_OK == SSL_get_verify_result(sslptr)){
			char buf[8192];
			X509_NAME *name = X509_get_subject_name(x);
			X509_NAME_oneline(name,buf,sizeof(buf)-1);
			s->peer_cert_info = buf;
		}
#endif
		s->pool = pool;
		if(s->init()){
			/*非阻塞模式*/
			unsigned int argp = 1;
			ioctl(acceptfd,FIONBIO,&argp);
			assert(pool);
			pool->add(s,s->flag);
		}else{
			s->destory();
		}
		return true;
	}

	bool aiolistener::notify_out()
	{
		return false;
	}

	bool aiolistener::notify_hup()
	{
		return false;
	}

	bool aiolistener::notify_err()
	{
		return false;
	}

	bool aiolistener::notify_pri()
	{
		return false;
	}

	aiolistener::aiolistener() : sslctx(nullptr),use_ssl(false),ssl_self_adaptive(false)
	{
	}

	aiolistener::~aiolistener()
	{
		close();
#ifdef OPENSSL_FOUND
		if(sslctx){
			SSL_CTX_free((SSL_CTX*)sslctx);
		}
#endif
	}

	bool aiolistener::open(lyramilk::data::uint16 port)
	{
		if(fd() >= 0){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aiolistener.open") << lyramilk::kdict("打开监听套件字失败，因为该套接字己打开。") << std::endl;
			return false;
		}

		native_socket_type tmpsock = ::socket(AF_INET,SOCK_STREAM, IPPROTO_IP);
		if(tmpsock < 0){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aiolistener.open") << lyramilk::kdict("监听时发生错误：%s",strerror(errno)) << std::endl;
			return false;
		}

		sockaddr_in addr = {0};
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		int opt = 1;
		setsockopt(tmpsock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
		if(::bind(tmpsock,(const sockaddr*)&addr,sizeof(addr))){
			::close(tmpsock);
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aiolistener.open") << lyramilk::kdict("绑定地址(%s:%d)时发生错误：%s",inet_ntoa(addr.sin_addr),port,strerror(errno)) << std::endl;
			return false;
		}
		unsigned int argp = 1;
		ioctl(tmpsock,FIONBIO,&argp);


		int ret = listen(tmpsock,5);
		if(ret == 0){
			lyramilk::klog(lyramilk::log::debug,"lyramilk.netio.aiolistener.open") << lyramilk::kdict("监听：%d",port) << std::endl;
			//signal(SIGPIPE, SIG_IGN);
			fd(tmpsock);
			return true;
		}
		::close(tmpsock);
		return false;
	}

	bool aiolistener::open(const lyramilk::data::string& host,lyramilk::data::uint16 port)
	{
		if(fd() >= 0){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aiolistener.open") << lyramilk::kdict("打开监听套件字失败，因为该套接字己打开。") << std::endl;
			return false;
		}

		hostent* h = gethostbyname(host.c_str());
		if(h == nullptr){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.client.open") << lyramilk::kdict("获取IP地址失败：%s",strerror(errno)) << std::endl;
			return false;
		}

		in_addr* inaddr = (in_addr*)h->h_addr;
		if(inaddr == nullptr){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.client.open") << lyramilk::kdict("获取IP地址失败：%s",strerror(errno)) << std::endl;
			return false;
		}

		native_socket_type tmpsock = ::socket(AF_INET,SOCK_STREAM, IPPROTO_IP);
		if(tmpsock < 0){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aiolistener.open") << lyramilk::kdict("监听时发生错误：%s",strerror(errno)) << std::endl;
			return false;
		}

		sockaddr_in addr = {0};
		addr.sin_addr.s_addr = inaddr->s_addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		int opt = 1;
		setsockopt(tmpsock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
		if(::bind(tmpsock,(const sockaddr*)&addr,sizeof(addr))){
			::close(tmpsock);
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aiolistener.open") << lyramilk::kdict("绑定地址(%s:%d)时发生错误：%s",inet_ntoa(addr.sin_addr),port,strerror(errno)) << std::endl;
			return false;
		}
		unsigned int argp = 1;
		ioctl(tmpsock,FIONBIO,&argp);


		int ret = listen(tmpsock,5);
		if(ret == 0){
			lyramilk::klog(lyramilk::log::debug,"lyramilk.netio.aiolistener.open") << lyramilk::kdict("监听：%s:%d",host.c_str(),port) << std::endl;
			//signal(SIGPIPE, SIG_IGN);
			fd(tmpsock);
			return true;
		}
		::close(tmpsock);
		return false;
	}

	bool aiolistener::ssl_use_client_verify(bool force)
	{
#ifdef OPENSSL_FOUND
		if(sslctx == nullptr){
			lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.aiolistener.ssl.init_ssl") << lyramilk::kdict("设置客户端认证失败:%s",lyramilk::kdict("SSL未初始化").c_str()) << std::endl;
			return false;
		}
		if(force){
			SSL_CTX_set_verify((SSL_CTX*)sslctx,SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT,nullptr);
		}else{
			SSL_CTX_set_verify((SSL_CTX*)sslctx,SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE,nullptr);
		}
		return true;
#else
		return false;
#endif
	}

	bool aiolistener::ssl_load_verify_locations(const lyramilk::data::array& verify_locations)
	{
#ifdef OPENSSL_FOUND
		int r = 0;
		lyramilk::data::array::const_iterator it = verify_locations.begin();
		for(;it!=verify_locations.end();++it){
			r = SSL_CTX_load_verify_locations((SSL_CTX*)sslctx, it->str().c_str(), nullptr);
			if(r != 1) {
				lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.aiolistener.ssl.init_ssl") << lyramilk::kdict("设置可信证书失败:%s",ssl_err().c_str()) << std::endl;
				return false;
			}
		}
		return true;
#else
		return false;
#endif
	}

	bool aiolistener::init_ssl(const lyramilk::data::string& certfilename, const lyramilk::data::string& keyfilename)
	{
#ifdef OPENSSL_FOUND
		if(sslctx == nullptr){
			sslctx = SSL_CTX_new(SSLv23_server_method());
		}

		SSL_CTX_set_mode((SSL_CTX*)sslctx, SSL_MODE_AUTO_RETRY);
		SSL_CTX_set_mode((SSL_CTX*)sslctx, SSL_MODE_RELEASE_BUFFERS);

		int r = 0;
		if(!certfilename.empty()){
			r = SSL_CTX_use_certificate_file((SSL_CTX*)sslctx, certfilename.c_str(), SSL_FILETYPE_PEM);
			if(r != 1) {
				lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.aiolistener.ssl.init_ssl") << lyramilk::kdict("设置公钥失败:%s",ssl_err().c_str()) << std::endl;
				return false;
			}
		}
		if(!keyfilename.empty()){
			r = SSL_CTX_use_PrivateKey_file((SSL_CTX*)sslctx, keyfilename.c_str(), SSL_FILETYPE_PEM);
			if(r != 1) {
				lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.aiolistener.ssl.init_ssl") << lyramilk::kdict("设置私钥失败:%s",ssl_err().c_str()) << std::endl;
				return false;
			}
		}
		if(!certfilename.empty() && !keyfilename.empty()){
			r = SSL_CTX_check_private_key((SSL_CTX*)sslctx);
			if(r != 1) {
				lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.aiolistener.ssl.init_ssl") << lyramilk::kdict("验证密钥失败:%s",ssl_err().c_str()) << std::endl;
				return false;
			}
		}

		EC_KEY *ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
		SSL_CTX_set_tmp_ecdh((SSL_CTX*)sslctx, ecdh);
		EC_KEY_free(ecdh);
		SSL_CTX_set_tmp_rsa((SSL_CTX*)sslctx,RSA_generate_key(1024,RSA_F4,NULL,NULL));
		SSL_CTX_set_cipher_list( (SSL_CTX*)sslctx, "ALL" );
		const unsigned char pkey[] = "www.lyramilk.com";
		SSL_CTX_set_session_id_context((SSL_CTX*)sslctx,pkey,sizeof(pkey));

		SSL_CTX_set_options((SSL_CTX*)sslctx, SSL_OP_TLS_ROLLBACK_BUG);
		ssl(true);
		return true;
#else
		lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aiolistener.ssl.init_ssl") << lyramilk::kdict("不支持SSL") << std::endl;
		return false;
#endif
	}

	void aiolistener::ssl(bool use_ssl, bool ssl_self_adaptive)
	{
		this->use_ssl = use_ssl;
		this->ssl_self_adaptive = ssl_self_adaptive;
	}

	bool aiolistener::ssl()
	{
		return use_ssl && sslobj;
	}

	ssl_ctx_type aiolistener::get_ssl_ctx()
	{
#ifdef OPENSSL_FOUND
		if(sslctx == nullptr){
			sslctx = SSL_CTX_new(SSLv23_server_method());
		}
#endif
		return sslctx;
	}

	lyramilk::io::native_filedescriptor_type aiolistener::getfd()
	{
		return socket::fd();
	}

	
	void aiolistener::ondestory()
	{
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool udplistener::notify_in()
	{
		struct sockaddr_in addr = {};
		socklen_t addr_len = sizeof(addr);
		char buff[65536];
		int r = recvfrom(fd(),buff,sizeof(buff),0,(sockaddr*)&addr,&addr_len);

		lyramilk::data::ostringstream oss;
		if(onrequest(buff, r, oss,(sockaddr*)&addr)){
			lyramilk::data::string out = oss.str();
			r = sendto(fd(),out.c_str(),out.size(),0,(sockaddr*)&addr,addr_len);
			//lyramilk::klog(lyramilk::log::debug,"lyramilk.netio.udplistener.notify_in") << lyramilk::kdict("向(%s:%d)发送数据%llu个字节",inet_ntoa(addr.sin_addr),addr.sin_port,r) << std::endl;
		}

		return true;
	}

	bool udplistener::notify_out()
	{
		return true;
	}

	bool udplistener::notify_hup()
	{
		return true;
	}

	bool udplistener::notify_err()
	{
		return true;
	}

	bool udplistener::notify_pri()
	{
		return true;
	}

	udplistener::udplistener()
	{
	}

	udplistener::~udplistener()
	{
		close();
	}

	bool udplistener::open(lyramilk::data::uint16 port)
	{
		if(fd() >= 0){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.udplistener.open") << lyramilk::kdict("打开监听套件字失败，因为该套接字己打开。") << std::endl;
			return false;
		}

		native_socket_type tmpsock = ::socket(AF_INET,SOCK_DGRAM, 0);
		if(tmpsock < 0){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.udplistener.open") << lyramilk::kdict("监听时发生错误：%s",strerror(errno)) << std::endl;
			return false;
		}

		sockaddr_in addr = {0};
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		int opt = 1;
		setsockopt(tmpsock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
		if(::bind(tmpsock,(const sockaddr*)&addr,sizeof(addr))){
			::close(tmpsock);
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.udplistener.open") << lyramilk::kdict("绑定地址(%s:%d)时发生错误：%s",inet_ntoa(addr.sin_addr),port,strerror(errno)) << std::endl;
			return false;
		}
		lyramilk::klog(lyramilk::log::debug,"lyramilk.netio.udplistener.open") << lyramilk::kdict("监听：%d",port) << std::endl;
		fd(tmpsock);
		return true;
	}

	bool udplistener::open(const lyramilk::data::string& host,lyramilk::data::uint16 port)
	{
		if(fd() >= 0){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.udplistener.open") << lyramilk::kdict("打开监听套件字失败，因为该套接字己打开。") << std::endl;
			return false;
		}

		hostent* h = gethostbyname(host.c_str());
		if(h == nullptr){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.client.open") << lyramilk::kdict("获取IP地址失败：%s",strerror(errno)) << std::endl;
			return false;
		}

		in_addr* inaddr = (in_addr*)h->h_addr;
		if(inaddr == nullptr){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.client.open") << lyramilk::kdict("获取IP地址失败：%s",strerror(errno)) << std::endl;
			return false;
		}

		native_socket_type tmpsock = ::socket(AF_INET,SOCK_DGRAM, 0);
		if(tmpsock < 0){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.udplistener.open") << lyramilk::kdict("监听时发生错误：%s",strerror(errno)) << std::endl;
			return false;
		}

		sockaddr_in addr = {0};
		addr.sin_addr.s_addr = inaddr->s_addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		int opt = 1;
		setsockopt(tmpsock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
		if(::bind(tmpsock,(const sockaddr*)&addr,sizeof(addr))){
			::close(tmpsock);
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.udplistener.open") << lyramilk::kdict("绑定地址(%s:%d)时发生错误：%s",inet_ntoa(addr.sin_addr),port,strerror(errno)) << std::endl;
			return false;
		}
			lyramilk::klog(lyramilk::log::debug,"lyramilk.netio.udplistener.open") << lyramilk::kdict("监听：%s:%d",host.c_str(),port) << std::endl;
		fd(tmpsock);
		return true;
	}

	lyramilk::io::native_filedescriptor_type udplistener::getfd()
	{
		return socket::fd();
	}

	
	void udplistener::ondestory()
	{
	}

}}


//		virtual bool onrequest(const char* cache, int size, lyramilk::data::ostream& os) = 0;