#include "log.h"
#include "dict.h"
#include <sstream>
#include <time.h>
#include <iomanip>
#include <sys/file.h>

#ifdef __linux__
#include <unistd.h>
#endif
#include <stdio.h>
#include <linux/limits.h>

using namespace lyramilk::log;

logss lyramilk::klog;

logb::logb():daytime()
{
}

logb::~logb()
{
}

/*
std::size_t logb::strtime(char* buff,std::size_t buffsize,const char* fmt,time_t ti)
{
#ifdef __GNUC__
	tm __t;
	tm *t = localtime_r(&ti,&__t);
#else
	tm __t;
	tm* t = &__t;
	localtime_s(t, &ti);
#endif
	::strftime(buff,buffsize,fmt,t);
	return buff;
}
*/
void logb::log(time_t ti,type ty,const lyramilk::data::string& usr,const lyramilk::data::string& app,const lyramilk::data::string& module,const lyramilk::data::string& str) const
{
	char tmbuff[32];
	std::size_t r;
	tm __t;
	tm *t = localtime_r(&ti,&__t);



	{
		//输入日志时只用当前时间分页。
		time_t t_now = time(nullptr);
		tm __t;
		tm *t = localtime_r(&t_now,&__t);

		if(daytime.tm_year != __t.tm_year || daytime.tm_mon != __t.tm_mon || daytime.tm_mday != __t.tm_mday){
			daytime = __t;
			r = ::strftime(tmbuff,sizeof(tmbuff),"%F",t);
			lyramilk::data::string cache;
			cache.reserve(1024);
			cache.append("\x1b[36m  ======================= ");
			cache.append(tmbuff,r);
			cache.append(" =======================\x1b[0m\n");
			fwrite(cache.c_str(),cache.size(),1,stdout);
		}
	}


	r = ::strftime(tmbuff,sizeof(tmbuff),"%T",t);

	switch(ty){
	  case debug:{
		lyramilk::data::string cache;
		cache.reserve(1024);
		cache.append("\x1b[36m");
		cache.append(tmbuff,r);
		cache.append(" [");
		cache.append(module);
		cache.append("] ");
		cache.append(str);
		cache.append("\x1b[0m");
		fwrite(cache.c_str(),cache.size(),1,stdout);
	  }break;
	  case trace:{
		lyramilk::data::string cache;
		cache.reserve(1024);
		cache.append("\x1b[37m");
		cache.append(tmbuff,r);
		cache.append(" [");
		cache.append(module);
		cache.append("] ");
		cache.append(str);
		cache.append("\x1b[0m");
		fwrite(cache.c_str(),cache.size(),1,stdout);
	  }break;
	  case warning:{
		lyramilk::data::string cache;
		cache.reserve(1024);
		cache.append("\x1b[33m");
		cache.append(tmbuff,r);
		cache.append(" [");
		cache.append(module);
		cache.append("] ");
		cache.append(str);
		cache.append("\x1b[0m");
		fwrite(cache.c_str(),cache.size(),1,stdout);
	  }break;
	  case error:{
		lyramilk::data::string cache;
		cache.reserve(1024);
		cache.append("\x1b[31m");
		cache.append(tmbuff,r);
		cache.append(" [");
		cache.append(module);
		cache.append("] ");
		cache.append(str);
		cache.append("\x1b[0m");
		fwrite(cache.c_str(),cache.size(),1,stderr);
	  }break;
	}
}


logf::logf(const lyramilk::data::string& filefmt)
{
	this->filefmt = filefmt;
	fd = -1;
}

logf::~logf()
{
	if(fd != -1){
		::close(fd);
	}
}


bool logf::ok()
{
	if(fd == -1){
		char buff[32];
		std::size_t r;
		{
			//输入日志时只用当前时间分页。
			time_t t_now = time(nullptr);
			tm __t;
			tm *t = localtime_r(&t_now,&__t);

			if(daytime.tm_year != __t.tm_year || daytime.tm_mon != __t.tm_mon || daytime.tm_mday != __t.tm_mday){
				lyramilk::threading::mutex_sync _(lock);
				if(daytime.tm_year != __t.tm_year || daytime.tm_mon != __t.tm_mon || daytime.tm_mday != __t.tm_mday){
					daytime = __t;
					lyramilk::data::string newfilename;

					lyramilk::data::string::const_iterator it = filefmt.begin();
					for(;it!=filefmt.end();++it){
						if(*it == '?'){
							r = ::strftime(buff,sizeof(buff),"%F",t);
							newfilename.append(buff,r);
						}else{
							newfilename.push_back(*it);
						}
					}

					fd = open(newfilename.c_str(),O_WRONLY | O_CREAT | O_APPEND,0644);
				}
			}
		}
	}

	return fd != -1;
}

void logf::log(time_t ti,type ty,const lyramilk::data::string& usr,const lyramilk::data::string& app,const lyramilk::data::string& module,const lyramilk::data::string& str) const
{
	char buff[32];
	std::size_t r;
	{
		//输入日志时只用当前时间分页。
		time_t t_now = time(nullptr);
		tm __t;
		tm *t = localtime_r(&t_now,&__t);

		if(daytime.tm_year != __t.tm_year || daytime.tm_mon != __t.tm_mon || daytime.tm_mday != __t.tm_mday){
			lyramilk::threading::mutex_sync _(lock);
			if(daytime.tm_year != __t.tm_year || daytime.tm_mon != __t.tm_mon || daytime.tm_mday != __t.tm_mday){
				if(fd != -1){
					::close(fd);
				}
				daytime = __t;
				lyramilk::data::string newfilename;

				lyramilk::data::string::const_iterator it = filefmt.begin();
				for(;it!=filefmt.end();++it){
					if(*it == '?'){
						r = ::strftime(buff,sizeof(buff),"%F",t);
						newfilename.append(buff,r);
					}else{
						newfilename.push_back(*it);
					}
				}

				fd = open(newfilename.c_str(),O_WRONLY | O_CREAT | O_APPEND,0644);
			}
		}
	}

	if(fd == -1){
		//如果打开文件失败，则这一天不再输出日志。
		return;
	}

	static const char* typeconst[4] = {" [debug] "," [trace] "," [warning] "," [error] "};

	lyramilk::data::string cache;
	cache.reserve(1024);

	//写时间
	tm __t;
	tm *t = localtime_r(&ti,&__t);
	r = ::strftime(buff,sizeof(buff),"%T ",t);
	cache.append(buff,r);

	//写pid
	r = snprintf(buff,sizeof(buff),"%lu",(unsigned long)getpid());
	cache.append(buff,r);

	//写类型
	cache.append(typeconst[ty]);

	//写模块名
	cache.push_back('[');
	cache.append(module);
	cache.push_back(']');
	cache.push_back(' ');

	//写正文
	cache.append(str);

	write(fd,cache.c_str(),cache.size());
}


logbuf::logbuf(logss2& pp):buf(2048),p(pp)
{
	setp(buf.data(),buf.data() + buf.size());
}

logbuf::~logbuf()
{
}

logbuf::int_type logbuf::overflow(int_type _Meta)
{
	sync();
	return sputc(_Meta);
}

int logbuf::sync()
{
	const char* pstr = pbase();
	int len = (int)(pptr() - pbase());

	lyramilk::data::string module = p.module;

	static lyramilk::data::string exename;
	if(exename.empty()){
		char buff[PATH_MAX] = {0};
		ssize_t sz = readlink("/proc/self/exe",buff,PATH_MAX);
		if(sz > 0){
			exename.assign(buff,sz);
		}
	}

	static uid_t uid = geteuid() + 1;

	static lyramilk::data::string loginuser = "unknow_user";
	if(uid != geteuid()){
		uid = geteuid();
		loginuser =  getlogin();;
	}
	if(p.loger){
		p.loger->log(time(nullptr),p.t,loginuser.c_str(),exename.c_str(),module,lyramilk::data::string(pstr,len));
	}else{
		static logb* default_logb = new logb;
		default_logb->log(time(nullptr),p.t,loginuser.c_str(),exename.c_str(),module,lyramilk::data::string(pstr,len));
	}
	setp(buf.data(),buf.data() + buf.size());
	return 0;
}



logss2::logss2():db(*this)
{
	init(&db);
	t = trace;
}

logss2::~logss2()
{
}




logss::logss()
{
	p = nullptr;
}

logss::logss(const lyramilk::data::string& m)
{
	prefix = m;
	p = nullptr;
}

logss::logss(const logss& qlog,const lyramilk::data::string& m)
{
	prefix = m;
	p = qlog.p;
}

logss::~logss()
{}

void static logssclean(void* parg)
{
	logss2* plogss2 = (logss2*)parg;
	delete plogss2;
}

static pthread_key_t logss2_key;

static __attribute__ ((constructor)) void __init()
{
	pthread_key_create(&logss2_key,logssclean);
}

logss2& logss::operator()(type ty) const
{
	logss2* plogss2 = (logss2*)pthread_getspecific(logss2_key);
	if(!plogss2){
		plogss2 = new logss2;
		pthread_setspecific(logss2_key,plogss2);
	}
	plogss2->t = ty;
	plogss2->module = prefix;
	plogss2->loger = p;
	return *plogss2;
}

logss2& logss::operator()(const lyramilk::data::string& m) const
{
	logss2* plogss2 = (logss2*)pthread_getspecific(logss2_key);
	if(!plogss2){
		plogss2 = new logss2;
		pthread_setspecific(logss2_key,plogss2);
	}
	if(prefix.empty()){
		plogss2->module = m;
	}else{
		plogss2->module = prefix + "." + m;
	}
	plogss2->loger = p;
	return *plogss2;
}

logss2& logss::operator()(type ty,const lyramilk::data::string& m) const
{
	logss2* plogss2 = (logss2*)pthread_getspecific(logss2_key);
	if(!plogss2){
		plogss2 = new logss2;
		pthread_setspecific(logss2_key,plogss2);
	}
	plogss2->t = ty;
	if(prefix.empty()){
		plogss2->module = m;
	}else{
		plogss2->module = prefix + "." + m;
	}
	plogss2->loger = p;
	return *plogss2;
}

lyramilk::log::logb* logss::rebase(lyramilk::log::logb* ploger)
{
	if(ploger == p) return ploger;
	if(ploger){
		(*this)(lyramilk::log::debug,"lyramilk.log.logss") << lyramilk::kdict("切换日志状态") << std::endl;
	}else{
		(*this)(lyramilk::log::debug,"lyramilk.log.logss") << lyramilk::kdict("恢复默认日志状态") << std::endl;
	}

	logb* old = p;
	p = ploger;
	return old;
}
//		lyramilk::threading::mutex lock;

