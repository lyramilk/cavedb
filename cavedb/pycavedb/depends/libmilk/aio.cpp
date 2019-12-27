#include "netaio.h"
#include "dict.h"
#include "testing.h"
#include "log.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <cassert>
#include <stdlib.h>

namespace lyramilk{namespace io
{
	// aioselector
	aioselector::aioselector()
	{
		pool = nullptr;
		mask = 0;
	}

	aioselector::~aioselector()
	{}

	bool aioselector::notify_attach(aiopoll* container)
	{
		this->pool = container;
		return this->pool != nullptr;
	}

	bool aioselector::notify_detach(aiopoll* container)
	{
		assert(this->pool == container);
		if(this->pool == container){
			this->pool = NULL;
			return true;
		}
		return false;
	}

	bool aioselector::lock()
	{
		mlock.lock();
		if(this->pool){
			epoll_event ee;
			ee.data.ptr = this;
			ee.events = 0;
			if (epoll_ctl(this->pool->getfd(), EPOLL_CTL_DEL,getfd(), &ee) == -1) {
				mlock.unlock();
				return false;
			}
		}
		return true;
	}

	bool aioselector::unlock()
	{
		if(this->pool){
			epoll_event ee;
			ee.data.ptr = this;
			ee.events = mask;
			if (epoll_ctl(this->pool->getfd(), EPOLL_CTL_ADD,getfd(), &ee) == -1) {
				return false;
			}
		}
		mlock.unlock();
		return true;
	}

	// aiopoll
	bool aiopoll::transmessage()
	{
		const int ee_max = 32;
		epoll_event ees[ee_max];
		int ee_count = epoll_wait(getfd(), ees, ee_max,100);
		for(int i=0;i<ee_count;++i){
			epoll_event &ee = ees[i];
			aioselector* selector = (aioselector*)ee.data.ptr;
			if(selector){
				onevent(selector,ee.events);
			}
		}
		return true;
	}

	const static int pool_max = 1000000;

	aiopoll::aiopoll()
	{
		epfd = epoll_create(pool_max);
		fdcount = 0;
	}

	native_epool_type aiopoll::getfd()
	{
		return epfd;
	}

	aiopoll::~aiopoll()
	{
		if(epfd >= 0){
			::close(epfd);
		}
	}

	bool aiopoll::add(aioselector* r,lyramilk::data::int64 mask)
	{
		if(mask == -1) mask = EPOLLIN;
		assert(r);
		if(!r->mlock.test()){
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.add") << lyramilk::kdict("向epoll[%d]中添加套接字%d时发生错误%s",getfd(),r->getfd(),"事件己上锁") << std::endl;
			return false;
		}

		epoll_event ee;
		ee.data.ptr = r;
		ee.events = mask;
		r->mask = mask;

		if(!r->notify_attach(this)){
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.add") << lyramilk::kdict("向epoll[%d]中添加套接字%d时发生错误%s",getfd(),r->getfd(),"关联失败") << std::endl;
			return false;
		}
		if (epoll_ctl(getfd(), EPOLL_CTL_ADD, r->getfd(), &ee) == -1) {
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.add") << lyramilk::kdict("向epoll[%d]中添加套接字%d时发生错误%s",getfd(),r->getfd(),strerror(errno)) << std::endl;
			return false;
		}
		__sync_add_and_fetch(&fdcount,1);
		//r->mask = mask;	有bug。对象刚被add到池中就可能己经被删掉除了。
		return true;
	}

	bool aiopoll::reset(aioselector* r,lyramilk::data::int64 mask)
	{
		assert(r);
		epoll_event ee;
		ee.data.ptr = r;
		ee.events = mask;
		r->mask = mask;

		if (r->mlock.test() && epoll_ctl(getfd(), EPOLL_CTL_MOD, r->getfd(), &ee) == -1) {
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.reset") << lyramilk::kdict("修改epoll[%d]中的套接字%d时发生错误%s",getfd(),r->getfd(),strerror(errno)) << std::endl;
			return false;
		}
		//r->mask = mask;
		return true;
	}

	bool aiopoll::remove(aioselector* r)
	{
		assert(r);

		if(!r->mlock.test()){
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.remove") << lyramilk::kdict("从epoll[%d]中移除套接字%d时发生错误%s",getfd(),r->getfd(),"事件己上锁") << std::endl;
			return false;
		}

		epoll_event ee;
		ee.data.ptr = NULL;
		ee.events = 0;

		if (epoll_ctl(getfd(), EPOLL_CTL_DEL, r->getfd(), &ee) == -1) {
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.remove") << lyramilk::kdict("从epoll[%d]中移除套接字%d时发生错误%s",getfd(),r->getfd(),strerror(errno)) << std::endl;
			return false;
		}
		__sync_sub_and_fetch(&fdcount,1);
		r->mask = 0;
		r->ondestory();
		return true;
	}

	void aiopoll::onevent(aioselector* r,lyramilk::data::int64 events)
	{
		assert(r);
		if(events & EPOLLPRI){
			if(!r->notify_pri()){
				remove(r);
			}
			return;
		}else if(events & EPOLLIN){
			if(!r->notify_in()){
				remove(r);
			}
			return;
		}else if(events & EPOLLOUT){
			if(!r->notify_out()){
				remove(r);
			}
			return;
		}else if(events & (EPOLLHUP | EPOLLRDHUP)){
			if(!r->notify_hup()){
				remove(r);
			}
			return;
		}else if(events & EPOLLERR){
			if(!r->notify_err()){
				remove(r);
			}
			return;
		}
	}

	int aiopoll::svc()
	{
		while(running && transmessage());
		return 0;
	}

	lyramilk::data::int64 aiopoll::get_fd_count()
	{
		return fdcount;
	}

	//	aiopoll_safe
	aiopoll_safe::aiopoll_safe(std::size_t threadcount)
	{
		pthread_key_create(&seq_key,nullptr);
		thread_idx = 0;

		if(threadcount > 0){
			epfds.resize(threadcount);
			epfds[0].epfd = epfd;
			epfds[0].payload = 0;
			epfd = -1;

			for(std::size_t idx = 1;idx < epfds.size();++idx){
				epfds[idx].epfd = epoll_create(pool_max);
				epfds[idx].payload = 0;
			}


			lyramilk::threading::threads::active(threadcount);
		
		}
	}


	aiopoll_safe::~aiopoll_safe()
	{
		for(std::size_t idx = 0;idx < epfds.size();++idx){
			::close(epfds[idx].epfd);
		}
		pthread_key_delete(seq_key);
	}

	bool aiopoll_safe::add(aioselector* r,lyramilk::data::int64 mask)
	{
		return add(r,mask,false);
	}

	bool aiopoll_safe::add(aioselector* r,lyramilk::data::int64 mask,bool use_current_thread)
	{
		if(use_current_thread){
			return aiopoll::add(r,mask);
		}
		return add_to_thread(-1,r,mask);
	}

	bool aiopoll_safe::add_to_thread(lyramilk::data::int64 supper_idx,aioselector* r,lyramilk::data::int64 mask)
	{
		if(mask == -1) mask = EPOLLIN;
		if(supper_idx == -1){
			std::size_t min_idx = 0;
			std::size_t min_val = epfds[0].payload;
			for(std::size_t idx = 1;idx < epfds.size();++idx){
				if(epfds[idx].payload < min_val){
					min_val = epfds[idx].payload;
					min_idx = idx;
				}
			}
			supper_idx = min_idx;
		}

		epinfo& epi = epfds[supper_idx];
		epi.payload += 1000000;

		assert(r);
		if(!r->mlock.test()){
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.aiopoll_safe.add") << lyramilk::kdict("向epoll[%d]中添加套接字%d时发生错误%s",epi.epfd,r->getfd(),"事件己上锁") << std::endl;
			return false;
		}

		epoll_event ee;
		ee.data.ptr = r;
		ee.events = mask;
		r->mask = mask;
		if(!r->notify_attach(this)){
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.aiopoll_safe.add") << lyramilk::kdict("向epoll[%d]中添加套接字%d时发生错误%s",epi.epfd,r->getfd(),"关联失败") << std::endl;
			return false;
		}
		if (epoll_ctl(epi.epfd, EPOLL_CTL_ADD, r->getfd(), &ee) == -1) {
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.aiopoll_safe.add") << lyramilk::kdict("向epoll[%d]中添加套接字%d时发生错误%s",epi.epfd,r->getfd(),strerror(errno)) << std::endl;
			return false;
		}
		__sync_add_and_fetch(&fdcount,1);
		return true;
	}

	bool aiopoll_safe::remove_on_thread(lyramilk::data::int64 supper_idx,aioselector* r)
	{
		if(supper_idx < 0 || supper_idx >= (lyramilk::data::int64)epfds.size()){
			return false;
		}
		epinfo& epi = epfds[supper_idx];

		if(!r->mlock.test()){
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.remove") << lyramilk::kdict("从epoll[%d]中移除套接字%d时发生错误%s",getfd(),r->getfd(),"事件己上锁") << std::endl;
			return false;
		}

		epoll_event ee;
		ee.data.ptr = NULL;
		ee.events = 0;

		if (epoll_ctl(epi.epfd, EPOLL_CTL_DEL, r->getfd(), &ee) == -1) {
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.remove") << lyramilk::kdict("从epoll[%d]中移除套接字%d时发生错误%s",getfd(),r->getfd(),strerror(errno)) << std::endl;
			return false;
		}
		__sync_sub_and_fetch(&fdcount,1);
		r->mask = 0;
		r->ondestory();
		return true;
	}

	native_epool_type aiopoll_safe::getfd()
	{
		lyramilk::data::int64 idx = get_thread_idx();
		if(idx == -1) return -1;
		return epfds[idx].epfd;
	}

	lyramilk::data::int64 aiopoll_safe::get_thread_idx()
	{
		std::size_t seq = (std::size_t)(void*)pthread_getspecific(seq_key);
		if(seq < 1 || seq > epfds.size()) return -1;
		return seq - 1;
	}

	bool aiopoll_safe::active(std::size_t threadcount)
	{
		return false;
	}

	int aiopoll_safe::svc()
	{
		std::size_t seq = ++thread_idx;
		pthread_setspecific(seq_key,(void*)seq);
		epinfo& epi = epfds[seq - 1];

		lyramilk::debug::nsecdiff nd;
		while(running){
			epoll_event ee;
			int ee_count = epoll_wait(epi.epfd, &ee,1, 100);

			for(int i=0;i<ee_count;++i){
				aioselector* selector = (aioselector*)ee.data.ptr;
				if(selector){
					nd.mark();
					onevent(selector,ee.events);
					epi.payload += nd.diff();


					if(epi.payload > 0x7fffffff){
						std::size_t min_val = epfds[0].payload;
						for(std::size_t idx = 1;idx < epfds.size();++idx){
							if(epfds[idx].payload < min_val){
								min_val = epfds[idx].payload;
							}
						}

						for(std::size_t idx = 0;idx < epfds.size();++idx){
							__sync_fetch_and_sub(&epfds[idx].payload,min_val);
						}
					}

				}
			}
		}
		return 0;
	}


}}
