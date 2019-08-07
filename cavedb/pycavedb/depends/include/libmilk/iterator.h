#ifndef _lyramilk_data_iterator_h_
#define _lyramilk_data_iterator_h_

#include "config.h"
#include <map>
#include <iostream>

namespace lyramilk{ namespace data{

	template <typename C,typename T,typename L= ptrdiff_t>
	class output_iterator:public std::iterator<std::output_iterator_tag,T,L>
	{
	  public:
		virtual ~output_iterator()
		{}

		output_iterator()
		{}

		output_iterator(const C &o)
		{
			assign(o);
		}

		C& operator =(const C &o)
		{
			return assign(o);
		}

		bool operator ==(const C &o)
		{
			return equal(o);
		}

		bool operator !=(const C &o)
		{
			return !equal(o);
		}

		C& operator ++()
		{
			next();
			return *(C*)this;
		}

		C operator ++(int)
		{
			C c = *(C*)this;
			++*(C*)this;
			return c;
		}
		
		T& operator*()
		{
			return get();
		}

		T* operator->()
		{
			return get();
		}
	  protected:
		/// 获取当前数据的指针
		virtual T& get() = 0;
		/// 数据指针指向下一个迭代器
		virtual void next() = 0;
		/// 比较两个迭代器是否相等
		virtual bool equal(const C& c) const = 0;
		/// 以新的迭代器重置当前迭代器
		virtual C& assign(const C &o) = 0;
	};

	template <typename C,typename T,typename L=ptrdiff_t>
	class input_iterator:public std::iterator<std::output_iterator_tag,T,L>
	{
	  public:
		virtual ~input_iterator()
		{}

		input_iterator()
		{}

		input_iterator(const C &o)
		{
			assign(o);
		}

		C& operator =(const C &o)
		{
			return assign(o);
		}

		bool operator ==(const C &o)
		{
			return equal(o);
		}

		bool operator !=(const C &o)
		{
			return !equal(o);
		}

		const C& operator ++()
		{
			next();
			return *(C*)this;
		}

		C operator ++(int)
		{
			C c = *(C*)this;
			++*(C*)this;
			return c;
		}

		const T& operator*()
		{
			return get();
		}

		const T* operator->()
		{
			return &get();
		}
	  protected:
		/// 获取当前数据的指针
		virtual const T& get() const = 0;
		/// 数据指针指向下一个迭代器
		virtual void next() = 0;
		/// 比较两个迭代器是否相等
		virtual bool equal(const C& c) const = 0;
		/// 以新的迭代器重置当前迭代器
		virtual C& assign(const C &o) = 0;
	};

	template <typename C,typename T,typename L=ptrdiff_t>
	class foward_iterator:public std::iterator<std::forward_iterator_tag,T,L>
	{
	  public:
		virtual ~foward_iterator()
		{}

		foward_iterator()
		{}

		foward_iterator(const C &o)
		{
			assign(o);
		}

		C& operator =(const C &o)
		{
			return assign(o);
		}

		bool operator ==(const C &o)
		{
			return equal(o);
		}

		bool operator !=(const C &o)
		{
			return !equal(o);
		}

		C& operator ++()
		{
			next();
			return *(C*)this;
		}

		C operator ++(int)
		{
			C c = *(C*)this;
			++*(C*)this;
			return c;
		}

		T& operator*()
		{
			return get();
		}

		T* operator->()
		{
			return &get();
		}
	  protected:
		/// 获取当前数据的指针
		virtual T& get() = 0;
		/// 数据指针指向下一个迭代器
		virtual void next() = 0;
		/// 比较两个迭代器是否相等
		virtual bool equal(const C& c) const = 0;
		/// 以新的迭代器重置当前迭代器
		virtual C& assign(const C &o) = 0;
	};

	template <typename C,typename T,typename L=ptrdiff_t>
	class bidirectional_iterator:public std::iterator<std::bidirectional_iterator_tag,T,L>
	{
	  public:
		virtual ~bidirectional_iterator()
		{}

		bidirectional_iterator()
		{}

		bidirectional_iterator(const C &o)
		{
			assign(o);
		}

		C& operator =(const C &o)
		{
			return assign(o);
		}

		bool operator ==(const C &o)
		{
			return equal(o);
		}

		bool operator !=(const C &o)
		{
			return !equal(o);
		}

		C& operator ++()
		{
			next();
			return *(C*)this;
		}

		C operator ++(int)
		{
			C c = *(C*)this;
			++*(C*)this;
			return c;
		}

		C& operator --()
		{
			prev();
			return *(C*)this;
		}

		C operator --(int)
		{
			C c = *(C*)this;
			--*(C*)this;
			return c;
		}

		T& operator*()
		{
			return get();
		}

		T* operator->()
		{
			return &get();
		}
	  protected:
		/// 获取当前数据的指针
		virtual T& get() = 0;
		/// 数据指针指向下一个迭代器
		virtual void next() = 0;
		/// 数据指针指向上一个迭代器
		virtual void prev() = 0;
		/// 比较两个迭代器是否相等
		virtual bool equal(const C& c) const = 0;
		/// 以新的迭代器重置当前迭代器
		virtual C& assign(const C &o) = 0;
	};

	template <typename C,typename T,typename L=ptrdiff_t>
	class random_access_iterator:public std::iterator<std::random_access_iterator_tag,T,L>
	{
	  public:
		virtual ~random_access_iterator()
		{}

		random_access_iterator()
		{}

		random_access_iterator(const C &o)
		{
			assign(o);
		}

		C& operator =(const C &o)
		{
			return assign(o);
		}

		bool operator ==(const C &o)
		{
			return equal(o);
		}

		bool operator !=(const C &o)
		{
			return !equal(o);
		}

		C& operator ++()
		{
			L ci = index() + 1;
			to(ci);
			return *(C*)this;
		}

		C operator ++(int)
		{
			C c = *(C*)this;
			++*(C*)this;
			return c;
		}

		C& operator --()
		{
			L ci = index() - 1;
			to(ci);
			return *(C*)this;
		}

		C operator --(int)
		{
			C c = *(C*)this;
			--*(C*)this;
			return c;
		}

		C operator +(L i)
		{
			C c = *(C*)this;
			return c+=i;
		}

		C operator -(L i)
		{
			C c = *(C*)this;
			return c-=i;
		}

		C& operator +=(L i)
		{
			L ci = index() + i;
			to(ci);
			return *(C*)this;
		}

		C& operator -=(L i)
		{
			L ci = index() - i;
			to(ci);
			return *(C*)this;
		}

		bool operator <(const C &o);
		bool operator >(const C &o);

		bool operator <=(const C &o);
		bool operator >=(const C &o);

		T* operator [](L i)
		{
			L ci = 1;
			to(ci);
			return *(C*)this;
		}

		T& operator*()
		{
			return get();
		}

		T* operator->()
		{
			return &get();
		}
	  protected:
		/// 获取当前数据的指针
		virtual T& get() = 0;
		/// 获取当前数据的指针
		/// 将当前迭代器指向指定索引位置
		virtual void to(L i) = 0;
		/// 取得当前数据指针的索引值
		virtual L index() const = 0;
		/// 比较两个迭代器是否相等
		virtual bool equal(const C& c) const = 0;
		/// 以新的迭代器重置当前迭代器
		virtual C& assign(const C &o) = 0;
	};
}}



#endif
