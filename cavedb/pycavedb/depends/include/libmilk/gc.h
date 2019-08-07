#ifndef _lyramilk_gc_
#define _lyramilk_gc_

#include "config.h"
#include <cstddef>

/**
	@namespace lyramilk
	@brief lyramilk库基础命名空间。
*/
namespace lyramilk
{
	/**
	 *  @brief 智能指针（需配合obj使用）
	 *  @brief ptr将抽象为obj的指针，但它带有内存回收功能，使得赋予ptr的指针会被自动delete掉，因此不要对它进行delete操作。引用计数在obj上面维护，因此只适用于从obj继承的对象。对于常规的对象如有智能指针的需求请使用标准库的智能指针。
	 */
	template <typename T>
	class ptr
	{
	  public:
		/**
		 *  @brief 检查指针是否有效。
		 *  @details ptr将抽象为T的指针，但它带有内存回收功能，使得赋予ptr的指针会被自动delete掉，因此不要对它进行delete操作。引用计数在obj上面维护，因此只适用于从obj继承的对象。对于常规的对象如有智能指针的需求请使用标准库的智能指针。
		 */
		bool verify() const;
		/**
			@brief 构造函数。
			@details 产生一个空的智能指针。
		*/
		ptr();
		/**
			@brief 使用T的子类对象来构造。
			@details 将ptr设置为T类对象right的指针。在构造时将增加right的引用计数。
			@param right T类对象的指针。
		*/
		ptr(T* right);
		/**
			@brief 使用obj的智能指针ptr来构造。
			@details 使ptr引用right，两者共享同一个对象。
			@param right T类对象的智能指针。
		*/
		ptr(const ptr& right);
		/**
			@brief 析构函数。
			@details 减少被引用对角的引用计数，当引用计数为0时会尝试销毁对象。
		*/
		virtual ~ptr();

		/**
			@brief 类型转换函数。
			@details 当T是K子类时，转换该智能指针为K类型指针。
			@return 转换后的基类指针。
		*/
		template <typename K>
		K* as();

		/**
			@brief 获取指针。
		*/
		T* c_ptr();

		/**
			@brief 隐式类型转换操作符。获得基类K的指针。
			@details 当T是K子类时，可以将ptr<T>隐式转换为ptr<K>。
		*/
		template <typename K>
		operator ptr<K>();

		/**
			@brief 隐式类型转换操作符。获得T指针。
			@details 获得T指针。
		*/
		operator T*();

		/**
			@brief 隐式类型转换操作符。获得T常量指针。
			@details 获得T常量指针。
		*/
		operator const T*() const;

		/**
			@brief 指针操作符。模拟指针操作时提供标准对象指针。
			@details 模拟指针操作时提供标准对象指针。
		*/
		T* operator ->();

		/**
			@brief 指针操作符。模拟指针操作时提供常量对象指针。
			@details 模拟指针操作时提供常量对象指针。
		*/
		const T* operator ->() const;

		/**
			@brief 赋值操作符。用ptr引用T类对象的指针p。
			@details 用ptr引用T类对象的指针p。
			@param p T类对象的指针。
		*/
		T* operator=(T* p);

		/**
			@brief 指针操作符。用ptr引用T类对象的智能指针right。
			@details 用ptr引用T类对象的指针right。
			@param right T类对象的智能指针。
		*/
		ptr& operator=(const ptr& right);
	  private:
		T* _p;
	};

	/**
		@brief 支持引用计数的基础对象。
	*/
	class _lyramilk_api_ obj
	{
	  protected:
		/**
			@brief 销毁对象。
		*/
		virtual void ondestory();
	  public:
		/**
			@brief 构造函数
		*/
		obj();

		/**
			@brief 析构函数
		*/
		virtual ~obj();

		/**
			@brief 增加引用。
		*/
		virtual void add_ref();

		/**
			@brief 减少引用。
		*/
		virtual void sub_ref();

		/**
			@brief 对象引用计数。
			@return 返回对象引用计数。
		*/
		virtual int payload() const;

		/**
			@brief 反应对象是否有效。
			@return 对象无效时返回false。
		*/
		virtual bool verify() const;

		/**
			@brief 尝试销毁对象。
			@details 当引用计数为0时，销毁对象，否则什么也不做。
			@return 对象无效时返回false。
		*/
		virtual bool try_del();

		/**
			@brief new操作符，为了可以顺利地delete。
		*/
		void* operator new(size_t sz);
		void* operator new[](size_t sz);
		void operator delete(void*);
		void operator delete[](void*);
	  private:
		mutable long long _rc;
	};

	//  以下是lang::ptr的实现。
	template <typename T>
	bool ptr<T>::verify() const
	{
		return _p && _p->verify();
	}

	template <typename T>
	ptr<T>::ptr()
	{
		_p = nullptr;
	}

	template <typename T>
	ptr<T>::ptr(T* right)
	{
		_p = nullptr;
		if(right && right->verify()){
			_p = right;
			_p->add_ref();
		}
	}

	template <typename T>
	ptr<T>::ptr(const ptr& right)
	{
		_p = nullptr;
		operator=(right);
	}

	template <typename T>
	ptr<T>::~ptr()
	{
		if(_p){
			_p->sub_ref();
			_p->try_del();
		}
	}

	template <typename T>
	template <typename K>
	K* ptr<T>::as()
	{
		return (K*)_p;
	}

	template <typename T>
	T* ptr<T>::c_ptr()
	{
		return _p;
	}

	template <typename T>
	template <typename K>
	ptr<T>::operator ptr<K>()
	{
		return (K*)_p;
	}

	template <typename T>
	ptr<T>::operator T*()
	{
		return _p;
	}

	template <typename T>
	ptr<T>::operator const T*() const
	{
		return _p;
	}

	template <typename T>
	T* ptr<T>::operator ->()
	{
		return _p;
	}

	template <typename T>
	const T* ptr<T>::operator ->() const
	{
		return _p;
	}

	template <typename T>
	T* ptr<T>::operator=(T* p)
	{
		if(_p == p) return _p;
		if(p && p->verify()){
			p->add_ref();
		}
		if(_p){
			_p->sub_ref();
			_p->try_del();
		}
		_p = p;
		return _p;
	}

	template <typename T>
	ptr<T>& ptr<T>::operator=(const ptr<T>& right)
	{
		if(_p == right._p) return *this;
		if(right.verify()){
			right._p->add_ref();
		}
		if(_p){
			_p->sub_ref();
			_p->try_del();
		}
		_p = right._p;
		return *this;
	}
}
#endif

