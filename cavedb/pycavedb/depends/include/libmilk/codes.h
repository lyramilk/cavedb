#ifndef _lyramilk_data_codes_h_
#define _lyramilk_data_codes_h_
#include <vector>

#include "var.h"
#include "exception.h"

namespace lyramilk{ namespace data
{
	/**
		@brief 编码转换异常
	*/
	class _lyramilk_api_ coding_exception :public lyramilk::exception
	{
	  public:
		/**
			@brief 用一个字符串构造。在what中会返回该字符串。
			@param msg 在what中返回的字符串。
		*/
		coding_exception(const lyramilk::data::string& msg = "");
	};

	/**
		@brief 字符串编码转换
		@param str 源串
		@param from 源串编码
		@param to 目标串编码
	*/
	lyramilk::data::string _lyramilk_api_ iconv(const lyramilk::data::string& str, const lyramilk::data::string& from, const lyramilk::data::string& to);

	/**
		@brief 代表一种编码解析器。
		@details 每一个coding的实现表示原编码到一种目标编码的转换。在处理字符串编码转换问题上，会将输入编码默认为windows上的gbk或linux上的utf8。
	*/
	class _lyramilk_api_ coding
	{
	  public:
		virtual ~coding();
		/**
			@brief 字符串解码，将字符串由目标编码转换为原编码或格式。
			@param str 被转换的字符串。
			@return str转换为原串。
		*/
		virtual lyramilk::data::string decode(const lyramilk::data::string& str) = 0;
		/**
			@brief 字符串编码，将字符串或二进制串转换为目标编码或格式。
			@param str 被转换的字符串。
			@return 由str编码而来的新字符串。
		*/
		virtual lyramilk::data::string encode(const lyramilk::data::string& str) = 0;
	};


	/**
		@brief 编码转换工具；或压缩解压缩工具；或加密解密工具。
		@details codes以一个字符串描述一种格式转换方法。然后提供以这种字符串为参数的描述对一种数据格式与其原格式做转换。
	*/
	class _lyramilk_api_ codes
	{
	  public:
		/**
			@brief 编码对象获取函数。该函数用于向codes注册新编码时，提供新编码的对象。
		*/
		typedef coding* (*getter)();
		/**
			@brief 全局实例。
		*/
		static codes* instance();

		/**
			@brief 定义一种编码及该编码的转换对象。
		*/
		bool define(const lyramilk::data::string& codingname,getter gtr);
		/**
			@brief 取消定义一种编码。
		*/
		bool undefine(const lyramilk::data::string& codingname);

		/**
			@brief 获取解码器
			@param codingname 用该名称代表的编码对src进行转换。
		*/
		coding* getcoder(const lyramilk::data::string& codingname);

		/**
			@brief 将字符串转换为目标编码。
			@param src 将要被编码的字符换或二进制串。
			@param codingname 用该名称代表的编码对src进行转换。
			@return 编码后的新数据。
		*/
		lyramilk::data::string encode(const lyramilk::data::string& codingname,const lyramilk::data::string& src) throw(lyramilk::exception);

		/**
			@brief 将字符串转换为原编码。
			@param src 被编码过的字符串或二进制。
			@param codingname 编码名称。
			@return 解码后的原编码。
		*/
		lyramilk::data::string decode(const lyramilk::data::string& codingname,const lyramilk::data::string& src) throw(lyramilk::exception);

		/**
			@brief 以字符串数组形式返回所有支持的编码。
		*/
		lyramilk::data::array supports();
	  protected:
		typedef class _lyramilk_api_ std::map<lyramilk::data::string,getter> builder_type;
		builder_type builder;
	};
}}
#endif
