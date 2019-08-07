#ifndef _lyramilk_data_bigint32_h_
#define _lyramilk_data_bigint32_h_

#include "config.h"
#include "var.h"
#include <vector>
#include <iostream>
#include <assert.h>

namespace lyramilk{ namespace data
{
	/**
		@brief 大整数
		@details 小端大整数，支持无限数量级的大整数运算。
	*/
	class _lyramilk_api_ bigint
	{
	  public:
		class bitset
		{
			bigint& x;
			unsigned int& i;
			unsigned int p;
		  public:
			bitset(bigint& t,unsigned int& o,unsigned int pos);
			bitset& operator=(bool v);
			bitset& operator=(const bitset& b);
			bool operator~() const;
			operator bool() const;
		};
		typedef class _lyramilk_api_ std::vector<unsigned int> bytes_type;
		typedef std::size_t bindex_type;
		bytes_type d;
		bool minus() const;
		void bits(bindex_type bitcount);
		std::size_t bits() const;
		void trim();
		int cmp(const bytes_type& o) const;
		int cmp(const bigint& o) const;

		bigint opposite() const;

	  public:
		/**
			@brief 构造函数
			@details 通过一个整数构造
		*/
		bigint();

		/**
			@brief 构造函数
			@details 通过一个整数构造
			@param o 用于构造bigint的整数
		*/
		bigint(int o);

		/**
			@brief 构造函数
			@details 通过一个整数构造
			@param o 用于构造bigint的整数
		*/
		bigint(long o);

		/**
			@brief 构造函数
			@details 通过一个整数构造
			@param o 用于构造bigint的整数
		*/
		bigint(long long o);

		/**
			@brief 构造函数
			@details 通过一个整数构造
			@param o 用于构造bigint的整数
		*/
		bigint(unsigned int o);

		/**
			@brief 构造函数
			@details 通过一个整数构造
			@param o 用于构造bigint的整数
		*/
		bigint(unsigned long o);

		/**
			@brief 构造函数
			@details 通过一个整数构造
			@param o 用于构造bigint的整数
		*/
		bigint(unsigned long long o);

		/**
			@brief 构造函数
			@details 通过一个整数构造
			@param o 用于构造bigint的整数
		*/
		bigint(const bigint& o);

		/**
			@brief 赋值运算符
			@details 为大整数赋值
			@param o 用于构造bigint的整数
		*/
		bigint& operator =(int o);

		/**
			@brief 赋值运算符
			@details 为大整数赋值
			@param o 用于构造bigint的整数
		*/
		bigint& operator =(long o);

		/**
			@brief 赋值运算符
			@details 为大整数赋值
			@param o 用于构造bigint的整数
		*/
		bigint& operator =(long long o);

		/**
			@brief 赋值运算符
			@details 为大整数赋值
			@param o 用于构造bigint的整数
		*/
		bigint& operator =(unsigned int o);

		/**
			@brief 赋值运算符
			@details 为大整数赋值
			@param o 用于构造bigint的整数
		*/
		bigint& operator =(unsigned long o);

		/**
			@brief 赋值运算符
			@details 为大整数赋值
			@param o 用于构造bigint的整数
		*/
		bigint& operator =(unsigned long long o);

		/**
			@brief 赋值运算符
			@details 为大整数赋值
			@param o 用于构造bigint的整数
		*/
		bigint& operator =(const bigint& o);

		/**
			@brief 数组取值运算符
			@details 通过数组取值运算符获取或设置大整数的某个二进制位。
			@param o 取大整数的第o位
			@return 取大整数的第o位，可以获取或设置该位。
		*/
		bitset operator[](bindex_type o);

		/**
			@brief 小于运算符
			@details 判断该大整数是否小于另外一个大整数
			@param o 操作数o
			@return 当该大整数小于o时返回true。
		*/
		bool operator < (const bigint& o) const;

		/**
			@brief 大于运算符
			@details 判断该大整数是否大于另外一个大整数
			@param o 操作数o
			@return 当该大整数大于o时返回true。
		*/
		bool operator > (const bigint& o) const;

		/**
			@brief 小于等于运算符
			@details 判断该大整数是否小于等于另外一个大整数
			@param o 操作数o
			@return 当该大整数小于等于o时返回true。
		*/
		bool operator <= (const bigint& o) const;

		/**
			@brief 大于等于运算符
			@details 判断该大整数是否大于等于另外一个大整数
			@param o 操作数o
			@return 当该大整数大于等于o时返回true。
		*/
		bool operator >= (const bigint& o) const;

		/**
			@brief 等于运算符
			@details 判断该大整数是否等于另外一个大整数
			@param o 操作数o
			@return 当该大整数等于o时返回true。
		*/
		bool operator == (const bigint& o) const;

		/**
			@brief 不等于运算符
			@details 判断该大整数是否不等于另外一个大整数
			@param o 操作数o
			@return 当该大整数不等于o时返回true。
		*/
		bool operator != (const bigint& o) const;

		/**
			@brief 加等运算符
			@details 使该大整数增加o
			@param o 操作数o
			@return 该大整数
		*/
		bigint& operator += (const bigint& o);

		/**
			@brief 加法运算符
			@details 计算该大整数加o的结果
			@param o 操作数o
			@return 该大整数加o的结果
		*/
		bigint operator + (const bigint& o) const;

		/**
			@brief 减等运算符
			@details 使该大整数减去o
			@param o 操作数o
			@return 该大整数
		*/
		bigint& operator -= (const bigint& o);

		/**
			@brief 减法运算符
			@details 计算该大整数减o的结果
			@param o 操作数o
			@return 该大整数减o的结果
		*/
		bigint operator - (const bigint& o) const;

		/**
			@brief 乘等运算符
			@details 使该大整数乘以o
			@param o 操作数o
			@return 该大整数
		*/
		bigint& operator *= (const bigint& o);

		/**
			@brief 乘法运算符
			@details 计算该大整数乘以o的结果
			@param o 操作数o
			@return 该大整数乘以o的结果
		*/
		bigint operator * (const bigint& o) const;

		/**
			@brief 除等运算符
			@details 使该大整数除以o
			@param o 操作数o
			@return 该大整数
		*/
		bigint& operator /= (const bigint& o);

		/**
			@brief 除法运算符
			@details 计算该大整数除以o的结果，并忽略小数部分。
			@param o 操作数o
			@return 该大整数除以o的结果
		*/
		bigint operator / (const bigint& o) const;

		/**
			@brief 模等运算符
			@details 使该大整数按o取模
			@param o 操作数o
			@return 该大整数
		*/
		bigint& operator %= (const bigint& o);

		/**
			@brief 取模运算符
			@details 计算该大整数按o取模的结果。
			@param o 操作数o
			@return 该大整数按o取模的结果
		*/
		bigint operator % (const bigint& o) const;


		/**
			@brief 递增运算符
			@details 使该大整数加1
			@return 该大整数
		*/
		bigint& operator ++();

		/**
			@brief 递增运算符
			@details 使该大整数加1
			@return 该大整数
		*/
		bigint operator ++(int);

		/**
			@brief 递减运算符
			@details 使该大整数减1
			@return 该大整数
		*/
		bigint& operator --();

		/**
			@brief 递减运算符
			@details 使该大整数减1
			@return 该大整数
		*/
		bigint operator --(int);
		

		/**
			@brief 取反运算符
			@details 计算该大整数按位取反
			@return 该大整数按位取反的结果
		*/
		bigint operator ~() const;

		/**
			@brief 逻辑取反运算符
			@details 判断该大整数是否为0
			@return 如果该大整数为0，则返回true
		*/
		bool operator !() const;


		/**
			@brief 异或等于运算符
			@details 使该大整数异或o
			@param o 操作数o
			@return 该大整数
		*/
		bigint& operator ^= (const bigint& o);

		/**
			@brief 异或运算符
			@details 计算该大整数与o异或的结果
			@param o 操作数o
			@return 该大整数与o异或的结果
		*/
		bigint operator ^ (const bigint& o) const;

		/**
			@brief 按位与等于运算符
			@details 使该大整数按位与o
			@param o 操作数o
			@return 该大整数
		*/
		bigint& operator &= (const bigint& o);

		/**
			@brief 按位与运算符
			@details 计算该大整数与o按位与的结果
			@param o 操作数o
			@return 该大整数与o按位与的结果
		*/
		bigint operator & (const bigint& o) const;

		/**
			@brief 左移等于运算符
			@details 使该大整数左移o位
			@param o 操作数o
			@return 该大整数
		*/
		bigint& operator <<= (int o);

		/**
			@brief 左移运算符
			@details 计算该大整数左移o位的结果
			@param o 操作数o
			@return 该大整数左移o位的结果
		*/
		bigint operator << (int o) const;

		/**
			@brief 右移等于运算符
			@details 使该大整数右移o位
			@param o 操作数o
			@return 该大整数
		*/
		bigint& operator >>= (int o);

		/**
			@brief 右移运算符
			@details 计算该大整数右移o位的结果
			@param o 操作数o
			@return 该大整数右移o位的结果
		*/
		bigint operator >> (int o) const;

		/**
			@brief 工具函数：计算x的n次方。
			@param x 底数
			@param n 指数
			@return 幂
		*/
		static bigint pow(const bigint& x,const bigint& n);

		/**
			@brief 工具函数：计算x的n次方按m取模。
			@details 这是个简便运算，因为适用于rsa而提供出来。
			@param x 底数
			@param n 指数
			@param m 模数
			@return x的n次方幂按m取模的值
		*/
		static bigint powmod(const bigint& x,const bigint& n,const bigint& m);


		friend _lyramilk_api_ std::ostream& operator << (std::ostream& o, const bigint& r);
		friend _lyramilk_api_ std::istream& operator >> (std::istream& o, bigint& r);

	};

	_lyramilk_api_ std::ostream& operator << (std::ostream& o,const bigint& r);
	_lyramilk_api_ std::istream& operator >> (std::istream& o, bigint& r);
}}
#endif
