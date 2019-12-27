#include "exception.h"

namespace lyramilk{
	exception::exception()throw()
	{
	}

	exception::exception(const lyramilk::data::string& msg) throw()
	{
		str = msg;
	}

	exception::~exception() throw()
	{
	}

	const char* exception::what() const throw()
	{
		return str.c_str();
	}

	notimplementexception::notimplementexception(const lyramilk::data::string& msg) throw() : exception(msg)
	{
	}
}