#ifndef _lyramilk_cryptology_md5_h_
#define _lyramilk_cryptology_md5_h_
#include <streambuf>
#include <sstream>
#include <vector>
#include "var.h"

#ifdef _MSC_VER
	#pragma warning(disable:4250)
#endif

/**
	@namespace lyramilk::cryptology
	@brief 加密算法
	@details 该命名空间描述加密算法。
*/
namespace lyramilk{ namespace cryptology
{
#ifdef WIN32
	template class _lyramilk_api_ std::vector < char >;
#endif

	/**
		@brief 一条md5生成的hash。
		@details 将其不完整地实现成了160位大整数，这样的语义是不恰当的，但为了实现简便故如此实现。
	 */
	struct _lyramilk_api_ md5_key
	{
		char key[16];

		md5_key();
		md5_key(const cryptology::md5_key& o);
		md5_key& operator =(const cryptology::md5_key& o);
		lyramilk::data::string str16();
		lyramilk::data::string str32();
	};

	/**
		@brief 用stl的流缓冲区对象封装md5算法，设计初衷是为下面的md5类服务。
	 */
	class _lyramilk_api_ md5_buf :public std::basic_streambuf<char>
	{
	  public:
		typedef struct { 
			unsigned int state[4]; 
			unsigned int count[2]; 
			unsigned char buffer[64]; 
		} ctx; 
		md5_buf();
		virtual int_type overflow(int_type c);
		virtual int_type underflow();
		virtual int sync();
	  protected:
		ctx context;
		std::vector<char> putbuf;
		std::vector<char> getbuf;
	};

	/**
		@brief 用stl的流封装md5算法。
	 */
	class _lyramilk_api_ md5 :public std::basic_iostream<char>
	{
		md5_buf sb;
	  public:
		md5();
		/**
			@brief 计算md5并以对象形式返回。
		 */
		md5_key get_key();

		/**
			@brief 计算16位md5并以字符串形式返回。
		 */
		lyramilk::data::string str16();

		/**
			@brief 计算32位md5并以字符串形式返回。
		 */
		lyramilk::data::string str32();
	};
}}

#endif