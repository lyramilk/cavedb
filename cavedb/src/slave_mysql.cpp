#include "slave_mysql.h"
#include <libmilk/dict.h>
#include <libmilk/log.h>
#include <libmilk/exception.h>
#include <libmilk/sha1.h>
#include <algorithm>


/*
#include "const.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <endian.h>
*/

struct mysql_pack
{
	unsigned int length:24;
	unsigned int seq:8;
};



namespace lyramilk{ namespace cave
{
	static lyramilk::log::logss log("lyramilk.cave.slave_mysql");


	slave_mysql::slave_mysql()
	{
		status = st_idle;
		psync_replid = "";
		psync_offset = 0;
		loadsum = 0;
	}

	slave_mysql::~slave_mysql()
	{
		status = st_stop;
	}

	lyramilk::data::uint64 slave_mysql::tell_offset()
	{
		return psync_offset;
	}

	void slave_mysql::slaveof(const lyramilk::data::string& host,lyramilk::data::uint16 port,const lyramilk::data::string& user,const lyramilk::data::string& pwd,lyramilk::data::string psync_replid,lyramilk::data::uint64 psync_offset,slave* peventhandler)
	{
		this->host = host;
		this->port = port;
		this->pwd = pwd;
		this->user = user;
		this->peventhandler = peventhandler;
		this->psync_replid = psync_replid;
		this->psync_offset = psync_offset;
		active(1);
	}

	lyramilk::data::string slave_mysql::hexmem(const void *p, int size)
	{
		int capacity = (size < 20)? 32 : (size * 1.2);
		lyramilk::data::string ret;
		ret.reserve(capacity);


/*
		const char* s = (const char*)p;
		static const char *hex = "0123456789abcdef";
		for(int i=0; i<size; i++){
			char c = s[i];
			switch(c){
				case '\r':
					ret.append("\\r");
					break;
				case '\n':
					ret.append("\\n");
					break;
				case '\t':
					ret.append("\\t");
					break;
				case '\\':
					ret.append("\\\\");
					break;
				case ' ':
					ret.push_back(c);
					break;
				default:
					if(c >= '!' && c <= '~'){
						ret.push_back(c);
					}else{
						ret.append("\\x");
						unsigned char d = c;
						ret.push_back(hex[d >> 4]);
						ret.push_back(hex[d & 0x0f]);
					}
					break;
			}
		}

*/
		const char* s = (const char*)p;
		static const char *hex = "0123456789abcdef";
		for(int i=0; i<size; i++){
			char c = s[i];
			switch(c){
				default:
					{
						ret.append(" ");
						unsigned char d = c;
						ret.push_back(hex[d >> 4]);
						ret.push_back(hex[d & 0x0f]);
					}
					break;
			}
		}
		return ret;
	}


	template<class T>
	void bytes_read(std::istream& is,T* t)
	{}

	template< >
	void bytes_read(std::istream& is,unsigned char* t)
	{
		is.read((char*)t,1);
	}

	template< >
	void bytes_read(std::istream& is,unsigned short* t)
	{
		is.read((char*)t,2);
	}

	template< >
	void bytes_read(std::istream& is,unsigned int* t)
	{
		is.read((char*)t,4);
	}

	template<  >
	void bytes_read(std::istream& is,lyramilk::data::string* t)
	{
		do{
			char c = is.get();
			if(c == 0){
				break;
			}
			t->push_back(c);
		}while(is);
	}

	unsigned long long mysql_read_long(std::istream& is)
	{
		char c = is.get();
		if(c < 251){
			return (unsigned char)c;
		}else if(c == 251){
			return 0;
		}else if(c == 252){
			unsigned long long result = 0;
			is.read((char*)&result,2);
			return result;
		}else if(c == 253){
			unsigned long long result = 0;
			is.read((char*)&result,3);
			return result;
		}else if(c == 254){
			unsigned long long result = 0;
			is.read((char*)&result,8);
			return result;
		}else{
			TODO();
		}

	}

	template<class T>
	void bytes_write(std::ostream& os,T t)
	{}

	template< >
	void bytes_write(std::ostream& os,unsigned char t)
	{
		os.write((char*)&t,1);
	}

	template< >
	void bytes_write(std::ostream& os,unsigned short t)
	{
		os.write((char*)&t,2);
	}

	template< >
	void bytes_write(std::ostream& os,unsigned int t)
	{
		os.write((char*)&t,4);
	}

	template<  >
	void bytes_write(std::ostream& os,lyramilk::data::string t)
	{

		os.write(t.c_str(),t.size());
		os.put(0);
	}

	template<  >
	void bytes_write(std::ostream& os,const char* t)
	{

		os << t;
		os.put(0);
	}


	lyramilk::data::string sha1str(lyramilk::data::string str)
	{
		lyramilk::cryptology::sha1 s1;
		s1 << str;

		lyramilk::data::string result = s1.get_key().str();
		transform(result.begin(), result.end(), result.begin(), toupper);
		return result;
	}

	lyramilk::cryptology::sha1_key sha1(lyramilk::data::string str)
	{
		lyramilk::cryptology::sha1 s1;
		s1 << str;
		return s1.get_key();
	}


	lyramilk::data::string upper(lyramilk::data::string str)
	{
		transform(str.begin(), str.end(), str.begin(), toupper);
		return str;
	}

	lyramilk::cryptology::sha1_key mysqlpassword(lyramilk::data::string str)
	{
		lyramilk::cryptology::sha1 s1;
		s1 << str;
		lyramilk::cryptology::sha1 s2;
		s2.write(s1.get_key().key,20);
		return s2.get_key();
	}

	lyramilk::data::string mysqlxor(lyramilk::data::string str1,lyramilk::data::string str2)
	{
		lyramilk::data::string result;
		if(str1.size() > str2.size()){
			for(std::size_t i=0;i<str1.size();++i){
				result.push_back(str1[i] ^ str2[i%str2.size()]);
			}
		}else{
			for(std::size_t i=0;i<str2.size();++i){
				result.push_back(str2[i] ^ str1[i%str1.size()]);
			}
		}
		return result;
	}


	bool slave_mysql::reconnect()
	{
		c.close();
		if(c.open(host,port)){
			is.init(&c);

			//	解析hello包
			unsigned int protocol = 0;
			unsigned int threadid = 0;

			unsigned short server_capabilities = 0;
			unsigned char server_language = 0;
			unsigned short server_status = 0;
			unsigned short extend_server_capabilities = 0;
			unsigned char authentication_plug_length = 0;

			lyramilk::data::string version;
			lyramilk::data::string slat1;
			lyramilk::data::string slat2;


			mysql_pack server_hello_pack;
			is.read((char*)&server_hello_pack,4);
			hexmem(&server_hello_pack,sizeof(server_hello_pack));

			std::vector<unsigned char> hello;
			hello.resize(server_hello_pack.length);


			bytes_read(is,(unsigned char*)&protocol);
			bytes_read(is,&version);
			bytes_read(is,&threadid);
			bytes_read(is,&slat1);
			bytes_read(is,&server_capabilities);
			bytes_read(is,&server_language);
			bytes_read(is,&server_status);
			bytes_read(is,&extend_server_capabilities);
			bytes_read(is,&authentication_plug_length);

			for(int i=0;i<10;++i){
				is.get();
			}
			bytes_read(is,&slat2);

			log(lyramilk::log::debug,"login") << D("协议:%d",protocol) << std::endl;
			log(lyramilk::log::debug,"login") << D("线程:%d",threadid) << std::endl;
			log(lyramilk::log::debug,"login") << D("版本:%s",version.c_str()) << std::endl;
			log(lyramilk::log::debug,"login") << D("slat1:%u,%s",slat1.size(),slat1.c_str()) << std::endl;
			log(lyramilk::log::debug,"login") << D("slat2:%u,%s",slat2.size(),slat2.c_str()) << std::endl;
			log(lyramilk::log::debug,"login") << D("server_capabilities:%x",server_capabilities) << std::endl;
			log(lyramilk::log::debug,"login") << D("extend_server_capabilities:%x",extend_server_capabilities) << std::endl;

			// 登录认证
			unsigned short client_capabilities = 0xa685;
			unsigned short extend_client_capabilities = 0x0007;
			unsigned int max_packet = 0x40000000;
			unsigned char charset = 8;

			lyramilk::data::string clientreserve23;
			clientreserve23.resize(22);

			lyramilk::data::ostringstream oss;
			bytes_write(oss,client_capabilities);
			bytes_write(oss,extend_client_capabilities);
			bytes_write(oss,max_packet);
			bytes_write(oss,charset);
			bytes_write(oss,clientreserve23);
			bytes_write(oss,user);
			if(pwd.empty()){
				bytes_write(oss,"");
			}else{
				lyramilk::cryptology::sha1_key mysqlpwdobj = mysqlpassword(pwd);
				lyramilk::data::string stage2hash(mysqlpwdobj.key,20);
				lyramilk::data::string stage1hash(sha1(pwd).key,20);

				lyramilk::cryptology::sha1_key keyobj = sha1(slat1 + slat2 + stage2hash);
				lyramilk::data::string key(keyobj.key,20);

				lyramilk::data::string finallypassword = mysqlxor(key,stage1hash);
				oss.put((char)finallypassword.size());
				oss.write(finallypassword.c_str(),finallypassword.size());
			}


			mysql_pack client_login_pack;
			client_login_pack.length = oss.str().size();
			client_login_pack.seq = server_hello_pack.seq + 1;

			c.write((char*)&client_login_pack,4);
			c.write(oss.str().c_str(),oss.str().size());

			is.read((char*)&server_hello_pack,4);

			unsigned char response_pack_type = is.get();
			if(response_pack_type == 0){
				// OK
				unsigned int affectedrows = mysql_read_long(is);
				unsigned short server_status = 0;
				bytes_read(is,&server_status);
				unsigned short warning_count = 0;
				bytes_read(is,&warning_count);
				log(lyramilk::log::debug,"login") << D("使用用户%s登录%s:%u成功",user.c_str(),host.c_str(),port) << std::endl;
				return true;
			}else if(response_pack_type == 0xff){
				// ERROR
				unsigned short error_code;
				bytes_read(is,&error_code);

				unsigned char know = is.get();
				char server_status[6] = {0};
				is.read(server_status,5);

				lyramilk::data::string msg;
				unsigned int len = server_hello_pack.length - 1 - 2 - 1 - 5;
				for(unsigned int i=0;i<len;++i){
					msg.push_back(is.get());
				}
				log(lyramilk::log::error,"login") << D("使用用户%s登录%s:%u失败,原因:%s",user.c_str(),host.c_str(),port,msg.c_str()) << std::endl;
			}else if(response_pack_type == 0xfe){
				// EOF
			}else if(response_pack_type >= 0x01 && response_pack_type <= 0xfa){
				// DATA
			}else{
				TODO();
			}
			return false;
		}
		return false;
	}

	bool slave_mysql::exec(const lyramilk::data::array& cmd,lyramilk::data::strings* ret)
	{
		if(push(cmd)){
			pop(ret);
			return true;
		}
		return false;
	}

	bool slave_mysql::push(const lyramilk::data::array& cmd)
	{
		TODO();
	}

	bool slave_mysql::pop(lyramilk::data::strings* ret)
	{
		TODO();
	}

	int slave_mysql::svc()
	{
		while(status != st_stop){
			try{
				if(!(is.good() && c.isalive())){
					reconnect();

				}
				if(c.check_read(5000)){
					lyramilk::data::strings reply;
					pop(&reply);
					if(reply.size() > 0){
						TODO();
					}
				}else{
					peventhandler->notify_idle(psync_replid,psync_offset);
				}
			}catch(lyramilk::exception& e){
				log(lyramilk::log::error,"psync.catch") << e.what() << std::endl;
			}
		}
		log(lyramilk::log::error,"psync") << D("同步线程退出") << std::endl;
		return 0;
	}

	void slave_mysql::proc_noop(lyramilk::data::uint64 seq)
	{
		peventhandler->notify_idle(psync_replid,psync_offset);
	}

	void slave_mysql::proc_copy(lyramilk::data::uint64 seq,char cmd,const char* p,std::size_t l,const lyramilk::data::strings& args)
	{
		TODO();
	}

	void slave_mysql::proc_sync(lyramilk::data::uint64 seq,char cmd,const char* p,std::size_t l,const lyramilk::data::strings& args)
	{
		TODO();
	}

}}
