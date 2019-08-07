#ifndef _lyramilk_exception_
#define _lyramilk_exception_

#include <exception>
#include "var.h"


namespace lyramilk
{
	/**
		@breif 基本的异常对象。
	*/
	class _lyramilk_api_ exception :public std::exception
	{
	  protected:
		lyramilk::data::string str;
	  public:
		/**
			@brief 无任何信息的构造函数
		*/
		exception() throw();
		/**
			@brief 只携带错误信息的异常
			@details 通过一个字符串来构造异常。
			@param str 格式化参数
		*/
		exception(const lyramilk::data::string& msg) throw();

		/**
			@breif 析构函数
		*/
		virtual ~exception() throw();

		/**
			@breif 返回异常信息。
			@details 覆盖了std::exception中的方法。
			@return 返回异常信息。
		*/
		virtual const char* what() const throw();
	};


	/**
		@breif 方法未实现。
	*/
	class _lyramilk_api_ notimplementexception :public exception
	{
	  public:
		notimplementexception(const lyramilk::data::string& msg = "") throw();
	};

}
#endif

