#ifndef _lyramilk_system_threading_thread_h_
#define _lyramilk_system_threading_thread_h_

#include "config.h"
#include <set>

#ifdef __linux__
	#include <pthread.h>
	#include <semaphore.h>
#elif defined WIN32
	#include <windows.h>
	typedef HANDLE pthread_t;
	typedef HANDLE pthread_mutex_t;
	typedef SRWLOCK pthread_rwlock_t;
#endif

/**
	@namespace lyramilk::threading
	@brief 线程
	@details 该命名空间描述线程相关操作
*/

namespace lyramilk{namespace threading
{
	/**
		@brief 线程组
		@details 线程组对象，激活后多线程执行svc函数。使用的时候继承这个对象然后覆盖svc函数就可以。
	*/
#ifdef WIN32
	template class _lyramilk_api_ std::set < pthread_t>;
#endif
	class _lyramilk_api_ threads
	{
	  protected:
		bool running;
	  protected:
		typedef int return_type;
		std::size_t cap;
		std::size_t cur;
		void* lock;
		typedef std::set<pthread_t> pool_type;

		pool_type m;
#ifdef WIN32
		static int __stdcall thread_task(threads* p);
#else
		static int thread_task(threads* p);
#endif
	  public:
		/**
			@brief 线程组构造
		*/
		threads();
		/**
			@brief 线程组析构
			@details 析构时会等待所有线程结束才会真正退出。
		*/
		virtual ~threads();

		/**
			@brief 激活线程组
			@details 激活时将启动多个线程执行svc
			@param threadcount 激活的线程数。
		*/
		virtual bool active(std::size_t threadcount);

		/**
			@brief 激活线程组
			@details 激活时将启动多个线程执行svc，启动的线程数目根据cpu核心数决定。
		*/
		virtual bool active();

		/**
			@brief 激活的线程数
		*/
		virtual std::size_t size();

		/**
			@brief 配置的线程数
		*/
		virtual std::size_t capacity();

		/**
			@brief 线程函数，激活线程时触发。
			@details 子类中该成员函数将在额外的线程中执行。
			@return 这个返回值将作为线程的返回值。
		*/
		virtual int svc() = 0;
	};


	/// 互斥锁基类
	class _lyramilk_api_ mutex_super
	{
	  public:
		virtual ~mutex_super();
		/*
			@brief 阻塞上锁
			@details 该操作会阻塞直到上锁成功
		*/
		virtual void lock() = 0;
		/// 解锁
		virtual void unlock() = 0;
		/*
			@brief 非阻塞上锁
			@details 是否上锁成功需要看返回值
		*/
		virtual bool try_lock() = 0;

		/// 测试锁是否可以加锁，但并不实际对锁进行操作
		virtual bool test() const = 0;
	};

	/// 互斥锁的自动操作类，该类对象在构造和析构的时候自动加锁解锁。
	class _lyramilk_api_ mutex_sync
	{
		mutex_super& _mutexobj;
	  public:
		mutex_sync(mutex_super& l);
		~mutex_sync();
	};

	/**
		@brief 自旋互斥锁对象
		@details 不可重入的互斥锁，这个锁会使CPU处于忙等待。
	*/
	class _lyramilk_api_ mutex_spin:public mutex_super
	{
	  public:
#ifdef __GNUC__
		typedef bool native_handle_type;
#elif defined _MSC_VER
		typedef long native_handle_type;
#endif
	  protected:
		native_handle_type locked;
	  public:
		mutex_spin();
		virtual ~mutex_spin();
		virtual void lock();
		virtual void unlock();
		virtual bool try_lock();
		virtual bool test() const;
	};

	/**
		@brief 互斥锁对象
		@details 可重入的互斥锁。
	*/
	class _lyramilk_api_ mutex_os:public mutex_super
	{
	  public:
		typedef pthread_mutex_t native_handle_type;
	  protected:
		native_handle_type handler;
	  public:
		mutex_os();
		virtual ~mutex_os();
		virtual void lock();
		virtual void unlock();
		virtual bool try_lock();
		virtual bool test() const;
	  public:
	};

	/**
		@brief 读写锁
		@details 在互斥锁的基础上允许读与读操作不互斥。
	*/
	class _lyramilk_api_ mutex_rw
	{
		mutex_super* pr;
		mutex_super* pw;
	  public:
		mutex_rw();
		virtual ~mutex_rw();

		mutex_rw(const mutex_rw& o);
		mutex_rw& operator =(const mutex_rw& o);
		/**
			@brief 申请读锁
			@details 申请读锁，被占用时等待一段时间。
			@return 成功时返回true
		*/
		mutex_super& r();

		/**
			@brief 申请读锁
			@details 申请读锁，被占用时等待一段时间。
			@return 成功时返回true
		*/
		mutex_super& w();
	  protected:
		pthread_rwlock_t lock;
	};

	/**
		@brief 固定次数的资源锁
		@details 同一线程多次进入计多次。
	*/
	class _lyramilk_api_ mutex_semaphore :public mutex_super
	{
		long long sigval;
	  protected:
		long long max_signal;
	  public:
		mutex_semaphore();
		virtual ~mutex_semaphore();
		virtual void set_max_signal(long long max_signal);
		virtual long long get_max_signal();
		virtual long long get_signal();

		virtual void lock();
		virtual void unlock();
		virtual bool try_lock();
		virtual bool test() const;
	};

	/// 线程结束时调用。
	void thread_cleanup_push(void (*routine)(void*),void* arg);
}}

#endif
