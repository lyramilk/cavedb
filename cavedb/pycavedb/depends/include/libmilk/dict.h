#ifndef _lyramilk_data_dict_h_
#define _lyramilk_data_dict_h_

#include "var.h"

/**
	@namespace lyramilk::data
	@brief 多语言
*/
namespace lyramilk{namespace data{

	/**
		@brief 字典
		@details 字典通过加载json格式数据文件提供翻译字符串的能力。
	*/
	class _lyramilk_api_ dict
	{
		dict* p;
		bool inner;
	  protected:
		lyramilk::data::map m;
	  public:
		dict();
		virtual ~dict();
		/**
			@brief 打开日志记录文件
			@return 返回true表示打开成功
		*/
		virtual bool load(const lyramilk::data::string& filename);

		/**
			@brief 翻译失败通知
			@param 无法翻译的串。
			@details 发现不能翻译的字符串时发出该通知，可以用该通知记录下未翻译的字符串，以辅助翻译。
		*/
		virtual void notify(const lyramilk::data::string& str);
		
		/**
			@brief 格式化并翻译字符串
			@param fmt 格式串
			@param ... 补充参数
			@return 目标串
		*/
		lyramilk::data::string operator ()(const char* fmt,...);

		/**
			@brief 格式化并翻译字符串
			@param fmt 格式串
			@param ... 补充参数
			@return 目标串
		*/
		lyramilk::data::string trans(const char* fmt,...);
		/**
			@brief 翻译字符串
			@param src 源串
			@return 目标串
		*/
		virtual lyramilk::data::string translate(const lyramilk::data::string& src);

		/**
			@brief 重定向字典
			@details 将字典重定向到默认的实现中。
			@param pdict 自定义的字典。
			@return 旧的字典。
		*/
		dict* tie(dict* pdict);
	};
}}

namespace lyramilk{
	/// 全局默认字典，可以通过这个字典对输出字符串进行翻译。
	extern _lyramilk_api_ lyramilk::data::dict kdict;
}

#ifdef _MSC_VER
	#define D(...) lyramilk::kdict(__VA_ARGS__)
#elif defined __GNUC__
	#define D(x...) lyramilk::kdict(x)
#else
	#define D(x)	
#endif

#endif
