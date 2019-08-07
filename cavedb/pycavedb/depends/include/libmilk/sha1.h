#ifndef _lyramilk_cryptology_sha1_h_
#define _lyramilk_cryptology_sha1_h_
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
		@brief 一条sha1生成的hash。
		@details 将其不完整地实现成了160位大整数，这样的语义是不恰当的，但为了实现简便故如此实现。
	 */
	struct _lyramilk_api_ sha1_key
	{
		char key[20];

		sha1_key();
		sha1_key(const cryptology::sha1_key& o);
		sha1_key& operator =(const cryptology::sha1_key& o);
		lyramilk::data::string str();
	};

	/**
		@brief 用stl的流缓冲区对象封装sha1算法，设计初衷是为下面的sha1类服务。
	 */
	class _lyramilk_api_ sha1_buf :public std::basic_streambuf<char>
	{
	  public:
		typedef struct { 
			unsigned int state[5]; 
			unsigned int count[2]; 
			unsigned char buffer[64]; 
		} ctx; 
		sha1_buf();
		virtual int_type overflow(int_type c);
		virtual int_type underflow();
		virtual int sync();
	  protected:
		ctx context;
		std::vector<char> putbuf;
		std::vector<char> getbuf;
	};

	/**
		@brief 用stl的流封装sha1算法。
	 */
	class _lyramilk_api_ sha1 :public std::basic_iostream<char>
	{
		sha1_buf sb;
	  public:
		sha1();


		/**
			@brief 计算sha1并以对象形式返回。
		 */
		sha1_key get_key();


		/**
			@brief 计算sha1并以字符串形式返回。
		 */
		lyramilk::data::string str();
	};
}}

#endif