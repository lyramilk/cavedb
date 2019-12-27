#ifndef _lyramilk_log_h_
#define _lyramilk_log_h_

#include "var.h"
#include "thread.h"
#include <sstream>

/**
	@namespace lyramilk::log
	@brief 日志组件命名空间
	@details 该命名空间提供支持控制台彩色打印并适用性良好的日志基类logb和将logb封装到C++流中的日志对象logss
*/
namespace lyramilk { namespace log
{
	/**
		@brief 日志类型
	*/
	enum type{
		/// 调试日志，发布版本可能会忽略这种日志。
		debug = 0,
		/// 跟踪日志，通过该种类日志打印正常的运行信息。
		trace = 1,
		/// 调试日志，程序运行参数不准确，程序运行不会受到影响，但输出可能不精确。
		warning = 2,
		/// 错误日志，程序运行参数不正确，程序运行可能会受到影响，输出不准确或有遗漏。
		error = 3
	};

	/**
		@brief 基本日志
		@details 提供基本日志功能。这个模块将日志输出到控制台。
	*/
	class _lyramilk_api_ logb
	{
		mutable tm daytime;
	  public:
		/**
			@brief 构造函数
		*/
		logb();
		~logb();
		/**
			@brief 记录日志。
			@details 提供基本日志功能。这个模块将日志输出到控制台。
			@param ti 时间，内部会调用strtime被格式化成字符串。
			@param ty 类型，调试信息的类别，例如trace。
			@param usr 当前用户。
			@param app 当前进程名。
			@param module 模块名。
			@param str 日志消息。

		*/
		virtual void log(time_t ti,lyramilk::log::type ty,const lyramilk::data::string& usr,const lyramilk::data::string& app,const lyramilk::data::string& module,const lyramilk::data::string& str) const;
	};

	/**
		@brief 基本文件日志
		@details 提供基本日志功能。这个模块将日志输出到文件。
	*/
	class _lyramilk_api_ logf:public logb
	{
	  protected:
		lyramilk::data::string filefmt;
		mutable int fd;
		mutable lyramilk::threading::mutex_os lock;
		mutable tm daytime;
	  public:
		/**
			@brief 构造函数
		*/
		logf(const lyramilk::data::string& filefmt);
		virtual ~logf();


		virtual bool ok();
		/**
			@brief 记录日志。
			@details 提供基本日志功能。这个模块将日志输出到控制台。
			@param ti 时间，内部会调用strtime被格式化成字符串。
			@param ty 类型，调试信息的类别，例如trace。
			@param usr 当前用户。
			@param app 当前进程名。
			@param module 模块名。
			@param str 日志消息。
		*/
		virtual void log(time_t ti,lyramilk::log::type ty,const lyramilk::data::string& usr,const lyramilk::data::string& app,const lyramilk::data::string& module,const lyramilk::data::string& str) const;
	};

	class _lyramilk_api_ logss2;
#ifdef WIN32
	template class _lyramilk_api_ lyramilk::data::vector<char, lyramilk::data::allocator<char> >;
#endif
	/**
		@brief 日志流的缓冲
	*/
	class _lyramilk_api_ logbuf : public std::basic_streambuf<char>
	{
		friend class logss2;
		std::vector<char,lyramilk::data::allocator<char> > buf;
		logss2& p;
	  public:
		/**
			@brief 构造函数，通过一个日志流来构造流缓冲。
			@param pp 日志流。
		*/
		logbuf(logss2& pp);
		/**
			@brief 析构函数
		*/
		virtual ~logbuf();
		/**
			@brief 继承于template std::basic_streambuf，当写入缓冲发生溢出时触发。
			@param _Meta 缓冲区溢出时正在写入的字符，这个字符尚未写入到缓冲区中，因此清理完缓冲区后需要将它写到缓冲区里。
		*/
		virtual int_type overflow(int_type _Meta);
		/**
			@brief 继承于template std::basic_streambuf，同步缓冲区时发生，对于日志流来说，同步就是将日志写入到日志系统中。默认的实现会写入到控制台。
		*/
		virtual int sync();
	};


	class _lyramilk_api_ logss2 : public lyramilk::data::ostringstream
	{
		logb* loger;
		logbuf db;
		friend class logbuf;
		friend class logss;
		lyramilk::data::string module;
		lyramilk::log::type t;

		logss2();
	  public:
		virtual ~logss2();
	};

	/**
		@brief 日志流
		@details 提供符合C++标准流的日志承载功能，默认输出到控制台，可以通过tie绑定一定输出到其它位置的logb实例。
	*/
	class _lyramilk_api_ logss
	{
		logb* p;
		lyramilk::data::string prefix;
	  public:
		/**
			@brief 构造函数。
		*/
		logss();
		/**
			@brief 构造函数，通过给定一个模块名来构造。
			@param m 这个流所服务的模块。
		*/
		logss(const lyramilk::data::string& m);
		/**
			@brief 构造函数，通过给定一个默认流来构造。
			@param qlog 默认的流。
			@param m 这个流所服务的模块。
		*/
		logss(const logss& qlog,const lyramilk::data::string& m);
		virtual ~logss();
		/**
			@brief 模拟一个函数。通过这个函数来设置日志类型。
			@details 例如
			@verbatim
				输出一条日志logss.(lyramilk::log::warning)  << "这是一条调试信息" << std::endl;
				将会输出黄色 [基准模块]   [时间] 这是一条调试信息
			@endverbatim
			@param ty 日志类型
			@return 返回日志流自身以方便 << 运算符表现。
		*/
		logss2& operator()(lyramilk::log::type ty) const;
		/**
			@brief 模拟一个函数。通过这个函数来设置子模块。
			@details 例如
			@verbatim
				输出一条日志logss.("成员函数")  << "这是一条调试信息" << std::endl;
				将会输出 [基准模块.成员函数]   [时间] 这是一条调试信息
			@endverbatim
			@param m 子模块名称
			@return 返回日志流自身以方便 << 运算符表现。
		*/
		logss2& operator()(const lyramilk::data::string& m) const;
		/**
			@brief 模拟一个函数。通过这个函数来设置日志类型和子模块。
			@details 例如
			@verbatim
				输出一条日志logss.(lyramilk::log::warning,"成员函数")  << "这是一条调试信息" << std::endl;
				将会输出黄色 [基准模块.成员函数]   [时间] 这是一条调试信息
			@endverbatim
			@param m 子模块名称
			@param ty 日志类型
			@return 返回日志流自身以方便 << 运算符表现。
		*/
		logss2& operator()(lyramilk::log::type ty,const lyramilk::data::string& m) const;
		/**
			@brief 将日志输出到指定的logb实现中。
			@details 将该日志流定向到指定的ploger中，可以改变日志的表现形式以及存储方式。
			@param ploger 自定义的logb，在里面可以改变日志的现实以存储动作。
			@return 旧的logb。
		*/
		logb* rebase(logb* ploger);
	};
}}


namespace lyramilk{
	/// 全局默认日志流，所有的日志最好通过这个流来初始化。
	extern  _lyramilk_api_ lyramilk::log::logss klog;
}

#endif
