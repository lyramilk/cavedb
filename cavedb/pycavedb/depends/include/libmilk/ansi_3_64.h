#ifndef _lyramilk_ansi_3_64_h_
#define _lyramilk_ansi_3_64_h_

#include "config.h"
#include <iostream>
/**
	@namespace lyramilk::ansi_3_64
	@brief ANSI 3.64控制码的封装
	@details 将ANSI 3.64的控制码封装为支持C++标准库的形式。
*/
namespace lyramilk{namespace ansi_3_64{
	/**
		@brief 重置控制台打印组状态。
		@details 示例
		@verbatim
			std::cout << lyramilk::ansi_3_64::reset << "正常文字" << std::endl;
		@endverbatim
	*/
	_lyramilk_api_ std::ostream& reset(std::ostream& os);
	/**
		@brief 将控制台字体设置为粗体。
		@details 示例
		@verbatim
			std::cout << lyramilk::ansi_3_64::blod << "粗体文字" << std::endl;
		@endverbatim
	*/
	_lyramilk_api_ std::ostream& blod(std::ostream& os);

	/**
		@brief 将控制台字体颜色设置为黑色。
		@details 示例
		@verbatim
			std::cout << lyramilk::ansi_3_64::black << "黑色文字" << std::endl;
		@endverbatim
	*/
	_lyramilk_api_ std::ostream& black(std::ostream& os);
	/**
		@brief 将控制台字体颜色设置为红色。
		@details 示例
		@verbatim
			std::cout << lyramilk::ansi_3_64::red << "红色文字" << std::endl;
		@endverbatim
	*/
	_lyramilk_api_ std::ostream& red(std::ostream& os);
	/**
		@brief 将控制台字体颜色设置为绿色。
		@details 示例
		@verbatim
			std::cout << lyramilk::ansi_3_64::green << "绿色文字" << std::endl;
		@endverbatim
	*/
	_lyramilk_api_ std::ostream& green(std::ostream& os);
	/**
		@brief 将控制台字体颜色设置为黄色。
		@details 示例
		@verbatim
			std::cout << lyramilk::ansi_3_64::yellow << "黄色文字" << std::endl;
		@endverbatim
	*/
	_lyramilk_api_ std::ostream& yellow(std::ostream& os);
	/**
		@brief 将控制台字体颜色设置为蓝色。
		@details 示例
		@verbatim
			std::cout << lyramilk::ansi_3_64::blue << "蓝色文字" << std::endl;
		@endverbatim
	*/
	_lyramilk_api_ std::ostream& blue(std::ostream& os);
	/**
		@brief 将控制台字体颜色设置为紫色。
		@details 示例
		@verbatim
			std::cout << lyramilk::ansi_3_64::magenta << "紫色文字" << std::endl;
		@endverbatim
	*/
	_lyramilk_api_ std::ostream& magenta(std::ostream& os);
	/**
		@brief 将控制台字体颜色设置为蓝绿色。
		@details 示例
		@verbatim
			std::cout << lyramilk::ansi_3_64::cyan << "蓝绿色文字" << std::endl;
		@endverbatim
	*/
	_lyramilk_api_ std::ostream& cyan(std::ostream& os);
	/**
		@brief 将控制台字体颜色设置为白色。
		@details 示例
		@verbatim
			std::cout << lyramilk::ansi_3_64::white << "白色文字" << std::endl;
		@endverbatim
	*/
	_lyramilk_api_ std::ostream& white(std::ostream& os);

	/**
		@brief 将控制台字体下划线开启。
		@details 示例
		@verbatim
			std::cout << lyramilk::ansi_3_64::underline << "下划文字" << std::endl;
		@endverbatim
	*/
	_lyramilk_api_ std::ostream& underline(std::ostream& os);
	/**
		@brief 将控制台字体下划线关闭。
		@details 示例
		@verbatim
			std::cout << lyramilk::ansi_3_64::underline << "下划文字" << lyramilk::ansi_3_64::underline_endl << "正常文字" << std::endl;
		@endverbatim
	*/
	_lyramilk_api_ std::ostream& underline_endl(std::ostream& os);

	/**
		@brief 将控制台字体闪烁特效设置为快。
		@details 示例
		@verbatim
			std::cout << lyramilk::ansi_3_64::blink_slow << "慢闪烁文字" << std::endl;
		@endverbatim
	*/
	_lyramilk_api_ std::ostream& blink_slow(std::ostream& os);
	/**
		@brief 将控制台字体闪烁特效设置为慢。
		@details 示例
		@verbatim
			std::cout << lyramilk::ansi_3_64::blink_rapid << "慢闪烁文字" << std::endl;
		@endverbatim
	*/
	_lyramilk_api_ std::ostream& blink_rapid(std::ostream& os);
	/**
		@brief 将控制台字体闪烁特效关闭。
		@details 示例
		@verbatim
			std::cout << lyramilk::ansi_3_64::blink_slow << "慢闪烁文字" << lyramilk::ansi_3_64::blink_endl << "正常文字" << std::endl;
		@endverbatim
	*/
	_lyramilk_api_ std::ostream& blink_endl(std::ostream& os);

}}

#endif

