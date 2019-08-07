#ifndef _lyramilk_system_threading_atom_h_
#define _lyramilk_system_threading_atom_h_

#include "config.h"
#include "thread.h"
#include <unistd.h>
#include <list>

/**
	@namespace lyramilk::threading
	@brief 线程
	@details 该命名空间描述线程相关操作
*/

namespace lyramilk{namespace threading
{
	template <typename T>
	class atomic
	{
		T t;
	  public:
		atomic():t(0)
		{}
		atomic(const atomic& o)
		{
			t = o.t;
		}
		atomic(T o)
		{
			t = o; 
		}

		operator T()
		{
			return t;
		}

		atomic& operator =(const atomic& o)
		{
			__sync_bool_compare_and_swap(&t,t,o.t);
			return *this;
		}

		atomic& operator =(const T& o)
		{
			__sync_bool_compare_and_swap(&t,t,o);
			return *this;
		}
		
		atomic& operator +=(const T& o)
		{
			__sync_fetch_and_add(&t,o);
			return *this;
		}
		
		atomic& operator -=(const T& o)
		{
			__sync_fetch_and_sub(&t,o);
			return *this;
		}
		
		atomic operator +(const T& o)
		{
			return atomic(t) += o;
		}
		
		atomic& operator -(const T& o)
		{
			return atomic(t) -= o;
		}
		
		atomic& operator ++()
		{
			__sync_add_and_fetch(&t,1);
			return *this;
		}
		
		atomic& operator --()
		{
			__sync_sub_and_fetch(&t,1);
			return *this;
		}
		
		atomic operator ++(int)
		{
			return __sync_fetch_and_add(&t,1);
		}
		
		atomic operator --(int)
		{
			return __sync_fetch_and_sub(&t,1);
		}
		
		bool operator ==(const T& o)
		{
			return __sync_bool_compare_and_swap(&t,o,t);
		}
		
		bool operator !=(const T& o)
		{
			return !__sync_bool_compare_and_swap(&t,o,t);
		}
	};

	template <typename T>
	class lockfreequeue
	{
		typedef unsigned char status_type;
		const static status_type s_free = 1;
		const static status_type s_lock = 2;
		const static status_type s_data = 3;
		struct node
		{
			status_type s;
			T data;
			node()
			{
				s = lockfreequeue::s_free;
			}
		};
		node* ring;
		unsigned long long headseq;
		unsigned long long ringsize;
		unsigned long long tailseq;
	  public:
		lockfreequeue()
		{
			ringsize = 8192;
			ring = new node[ringsize];
			headseq = 0;
			tailseq = 0;
		}
		lockfreequeue(unsigned long long ringcapacity)
		{
			ringsize = ringcapacity;
			ring = new node[ringsize];
			headseq = 0;
			tailseq = 0;
		}
		~lockfreequeue()
		{
			delete [] ring;
		}

		void push(const T& t)
		{
			while(true){
				unsigned int seq = tailseq;
				node& n = ring[seq%ringsize];
				if(__sync_bool_compare_and_swap(&n.s,s_free,s_lock)){
					if(__sync_bool_compare_and_swap(&tailseq,seq,seq + 1)){
						n.data = t;
						n.s = s_data;
						return;
					}
					n.s = s_free;
				}
				usleep(1);
			}
		}

		bool empty()
		{
			return headseq == tailseq;
		}

		std::size_t size()
		{
			return tailseq - headseq;
		}

		bool try_pop(T* t)
		{
			unsigned int seq = headseq;

			node& n = ring[seq%ringsize];
			if(__sync_bool_compare_and_swap(&n.s,s_data,s_lock)){
				if(__sync_bool_compare_and_swap(&headseq,seq,seq + 1)){
					*t = n.data;
					n.s = s_free;
					return true;
				}
				n.s = s_data;
			}
			return false;
		}

		void pop(T* t)
		{
			while(true){
				unsigned int seq = headseq;
				node& n = ring[seq%ringsize];
				if(__sync_bool_compare_and_swap(&n.s,s_data,s_lock)){
					if(__sync_bool_compare_and_swap(&headseq,seq,seq + 1)){
						*t = n.data;
						n.s = s_free;
						return;
					}
					n.s = s_data;
				}
				usleep(1);
			}
		}
	};

	template <typename T>
	class rentlist
	{
	  public:
		struct item
		{
			mutex_spin l;
			T* t;
			rentlist* c;
			int r;
			item(T* t,rentlist* c)
			{
				this->t = t;
				this->c = c;
				r = 0;
			}

			~item()
			{}

			bool trylock()
			{
				if(l.try_lock()){
					c->onhire(t);
					return true;
				}
				return false;
			}

			void lock()
			{
				l.lock();
				c->onhire(t);
			}

			bool addref()
			{
				__sync_add_and_fetch(&r,1);
				return true;
			}

			bool release()
			{
				if(__sync_sub_and_fetch(&r,1) < 1){
					c->onfire(t);
					l.unlock();
				}
				return true;
			}
		};

		class ptr
		{
			mutable item* q;
		  public:
			ptr():q(nullptr)
			{
			}

			ptr(const ptr& p):q(p.q)
			{
				if(q) q->addref();
			}

			ptr(item* pi):q(pi)
			{
				if(q) q->addref();
			}

			~ptr()
			{
				if(q) q->release();
			}

			bool good()
			{
				return q != NULL;
			}

			ptr& operator =(const ptr& o)
			{
				if(o.q) o.q->addref();
				if(q) q->release();
				q = o.q;
				return *this;
			}

			T* operator->() const
			{
				return q?q->t:nullptr;
			}

			operator T*() const
			{
				return q?q->t:nullptr;
			}
		};

		ptr get()
		{
			typename std::list<item>::iterator it = es.begin();
			for(;it!=es.end();++it){
				if(it->trylock()){
					return ptr(&*it);
				}
			}
			T* tmp = underflow();
			if(tmp){
				item* pe = nullptr;
				{
					mutex_sync _(l);
					es.push_back(item(tmp,this));
					pe = &es.back();
					pe->lock();
				}
				return ptr(pe);
			}
			return ptr(nullptr);
		}


		virtual void clear()
		{
			typename std::list<item>::iterator it = es.begin();
			for(;it!=es.end();++it){
				onremove(it->t);
			}
			es.clear();
			
		}
	  protected:
		virtual T* underflow() = 0;
		virtual void onhire(T* o)
		{}
		virtual void onfire(T* o)
		{}
		virtual void onremove(T* o)
		{}

		rentlist()
		{}

		virtual ~rentlist()
		{}

		typedef std::list<item> list_type;
		list_type es;
		mutex_spin l;
	};
}}

#endif
