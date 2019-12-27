#include "datawrapper.h"
#include "dict.h"
#include "exception.h"

#include <iconv.h>
#include <algorithm>


//namespace lyramilk { namespace data{
lyramilk::data::wstring inline utf8_unicode(const lyramilk::data::string& str)
{
	lyramilk::data::string dst = lyramilk::data::iconv(str,"utf8//ignore","wchar_t");
	if(dst.size()&1){
		dst.push_back(0);
	}
	return lyramilk::data::wstring((wchar_t*)dst.c_str(),dst.size() >> lyramilk::data::intc<sizeof(wchar_t)>::square);
}

lyramilk::data::string inline unicode_utf8(const lyramilk::data::wstring& str)
{
	lyramilk::data::string src((char*)str.c_str(),str.size() << lyramilk::data::intc<sizeof(wchar_t)>::square);
	lyramilk::data::string dst = lyramilk::data::iconv(src,"wchar_t","utf8");
	return dst;
}



lyramilk::data::datawrapper::datawrapper()
{
	
}

lyramilk::data::datawrapper::~datawrapper()
{
	
}

lyramilk::data::string lyramilk::data::datawrapper::get_str()
{
	throw lyramilk::notimplementexception(lyramilk::kdict("datawrapper(%s) 错误：未实现t_str类型的转换",name().c_str()));
}

lyramilk::data::datawrapper& lyramilk::data::datawrapper::at(lyramilk::data::uint64 index)
{
	throw lyramilk::notimplementexception(lyramilk::kdict("datawrapper(%s) 错误：未实现t_array类型的转换",name().c_str()));
}

lyramilk::data::datawrapper& lyramilk::data::datawrapper::at(const lyramilk::data::string& index)
{
	throw lyramilk::notimplementexception(lyramilk::kdict("datawrapper(%s) 错误：未实现t_map类型的转换",name().c_str()));
}

lyramilk::data::chunk lyramilk::data::datawrapper::get_bytes()
{
	lyramilk::data::string str = get_str();
	return lyramilk::data::chunk((const unsigned char*)str.c_str(),str.size());
}

lyramilk::data::wstring lyramilk::data::datawrapper::get_wstr()
{
	return utf8_unicode(get_str());
}

bool lyramilk::data::datawrapper::get_bool()
{
	lyramilk::data::string lowercasestr = get_str();
	std::transform(lowercasestr.begin(),lowercasestr.end(),lowercasestr.begin(),tolower);
	if(lowercasestr == "true") return true;
	if(lowercasestr == "false") return false;
	if(lowercasestr == "1") return true;
	if(lowercasestr == "0") return false;

	throw lyramilk::data::type_invalid(lyramilk::kdict("datawrapper(%s) 错误：无法转换为bool类型",name().c_str()));
}

lyramilk::data::int64 lyramilk::data::datawrapper::get_int()
{
	lyramilk::data::string str = get_str();
	if(str.size() > 0){
		char* p;
		lyramilk::data::int64 r = strtoll(str.c_str(),&p,10);
		if(p == str.c_str() + str.size()){
			return r;
		}
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("datawrapper(%s) 错误：无法转换为int64类型",name().c_str()));
}

double lyramilk::data::datawrapper::get_double()
{
	lyramilk::data::string str = get_str();
	if(str.size() > 0){
		char* p;
		double r = strtod(str.c_str(),&p);
		if(p == str.c_str() + str.size()){
			return r;
		}
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("datawrapper(%s) 错误：无法转换为double类型",name().c_str()));
}

lyramilk::data::datawrapper& lyramilk::data::datawrapper::at(const lyramilk::data::wstring& index)
{
	return at(unicode_utf8(index));
}

/*

lyramilk::data::pointer_datawrapper::pointer_datawrapper(void* p):ptr(p)
{
}

lyramilk::data::pointer_datawrapper::~pointer_datawrapper()
{
}

lyramilk::data::string lyramilk::data::pointer_datawrapper::class_name()
{
	return "lyramilk.datawrapper.pointer";
}

lyramilk::data::string lyramilk::data::pointer_datawrapper::name() const
{
	return class_name();
}

lyramilk::data::datawrapper* lyramilk::data::pointer_datawrapper::clone() const
{
	return new pointer_datawrapper(ptr);
}

void lyramilk::data::pointer_datawrapper::destory()
{
	delete this;
}

bool lyramilk::data::pointer_datawrapper::type_like(lyramilk::data::var::vt nt) const
{
	return false;
}
*/