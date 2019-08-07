#ifndef _lyramilk_data_stringutil_h_
#define _lyramilk_data_stringutil_h_

#include "config.h"
#include "var.h"

/**
	@namespace lyramilk::data
	@brief 数据
	@details 该命名空间描述数据的表达形式。
*/

namespace lyramilk{namespace data
{
	/**
		@brief 格式化字符串(这里并不进行翻译)
		@param fmt 格式串
		@param ... 补充参数
		@return 目标串
	*/
	lyramilk::data::string format(const char* fmt,...);

	/*
		@brief 分割字符串
		@param data 源串
		@param sep 分割符
	*/
	lyramilk::data::strings split(const lyramilk::data::string& data,const lyramilk::data::string& sep);

	/*
		@brief 分割文件路径
		@param data 源串
		@param sep 分割符
	*/
	lyramilk::data::strings path_split(const lyramilk::data::string& path);

	/*
		@brief 去掉字符串两端的指定字符
		@param data 源串
		@param pattern 需要去掉的字符列表
	*/
	lyramilk::data::string trim(const lyramilk::data::string& data,const lyramilk::data::string& pattern);

	/*
		@brief 去掉字符串左端的指定字符
		@param data 源串
		@param pattern 需要去掉的字符列表
	*/
	lyramilk::data::string ltrim(const lyramilk::data::string& data,const lyramilk::data::string& pattern);

	/*
		@brief 去掉字符串右端的指定字符
		@param data 源串
		@param pattern 需要去掉的字符列表
	*/
	lyramilk::data::string rtrim(const lyramilk::data::string& data,const lyramilk::data::string& pattern);

	/*
		@brief 大写字母转小写
	*/
	lyramilk::data::string lower_case(const lyramilk::data::string& src);


	/*
		@brief 大写字母转小写
	*/
	lyramilk::data::string upper_case(const lyramilk::data::string& src);



}}

#endif
