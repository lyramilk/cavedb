#include "var.h"
#include "dict.h"
#include "datawrapper.h"
#include <vector>
#include <sstream>
#include <algorithm>
#include <stdio.h>
#include <memory.h>

#ifndef null
#define null nullptr
#endif
#ifdef Z_HAVE_JEMALLOC
	#include <jemalloc/jemalloc.h>
#endif


#ifdef _WIN32
	#include <Windows.h>
#elif defined __linux__
	#include <iconv.h>
	#include <assert.h>
	#include <arpa/inet.h>
#endif

//namespace lyramilk { namespace data{

const lyramilk::data::var lyramilk::data::var::nil;

template <typename T>
T reverse_order(T t)
{
	const int sz = sizeof(T);
	union{
		T v;
		char c[sizeof(T)];
	}s,d;
	s.v = t;
	for(int i =0;i < sz;++i){
		d.c[i] = s.c[sz - i - 1];
	}
	return d.v;
}

#ifdef _WIN32
	class u2a
	{
		lyramilk::data::string p;
	  public:
		u2a(const lyramilk::data::wstring& wstr)
		{
			p.resize((wstr.length() + 1) << 1);
			char* _ = (char*)p.c_str();
			int i = WideCharToMultiByte(CP_ACP,0,wstr.c_str(),(int)wstr.length(),_,(int)p.capacity(),null,null);
			p.erase(p.begin() + i,p.end());
		}
		/*
		operator const char*()
		{
			return p.data();
		}*/
		operator lyramilk::data::string()
		{
			return p;
		}
	};

	class a2u
	{
		lyramilk::data::wstring p;
	  public:
		a2u(const lyramilk::data::string& str)
		{
			p.resize(str.length() + 1);
			wchar_t* _ = (wchar_t*)p.c_str();
			int i = MultiByteToWideChar(CP_ACP,0,str.c_str(),(int)str.length(),_,(int)p.capacity());
			p.erase(p.begin() + i,p.end());
		}
		/*
		operator const wchar_t*()
		{
			return p.data();
		}*/
		operator lyramilk::data::wstring()
		{
			return p;
		}
	};

	class u2t
	{
		lyramilk::data::string p;
	  public:
		u2t(const wstring& wstr)
		{
			int len = WideCharToMultiByte(CP_UTF8,0,wstr.c_str(),(int)wstr.length(),null,0,null,null);
			p.resize(len);
			char* _ = (char*)p.c_str();
			int i = WideCharToMultiByte(CP_UTF8,0,wstr.c_str(),(int)wstr.length(),_,(int)p.capacity(),null,null);
			p.erase(p.begin() + i,p.end());
		}
		/*
		operator const char*()
		{
			return p.c_str();
		}*/
		operator lyramilk::data::string()
		{
			return p;
		}
	};
	class t2u
	{
		wstring p;
	  public:
		t2u(const lyramilk::data::string& str)
		{
			int len = MultiByteToWideChar(CP_UTF8,0,str.c_str(),(int)str.length(),null,0);
			p.resize(len);
			wchar_t* _ = (wchar_t*)p.c_str();
			int i = MultiByteToWideChar(CP_UTF8,0,str.c_str(),(int)str.length(),_,(int)p.capacity());
			p.erase(p.begin() + i,p.end());
		}
		/*
		operator const wchar_t*()
		{
			return p.data();
		}*/
		operator wstring()
		{
			return wstring(p.data());
		}
	};

	#define t2a(x) (u2a(t2u(x)))
	#define a2t(x) (u2t(a2u(x)))

#elif defined __linux__
	lyramilk::data::string iconv(const lyramilk::data::string& str,const lyramilk::data::string& from,const lyramilk::data::string& to)
	{
		const size_t ds = 4096;
		iconv_t cd = iconv_open(to.c_str(),from.c_str());
		if(cd == 0){
			assert(cd);
			return "";
		}

		char* p1 = (char*)str.c_str();
		size_t p1s = str.size();
		std::vector<char> ret;
		char* p2 = ret.data();
		size_t p2s = ret.size();

		int rc = -1;
		do{
			size_t pos = p2 - ret.data();
			ret.insert(ret.end(),ds,0);
			p2 = ret.data() + pos;
			p2s = ret.size() - pos;
			rc = ::iconv(cd,&p1,&p1s,&p2,&p2s);
		}while(rc == -1 && p2s < ds);

		iconv_close(cd);
		if(rc == -1){
			return "";
		}
		return lyramilk::data::string(ret.data(),p2 - (char*)ret.data());
	}

	lyramilk::data::wstring inline utf8_unicode(const lyramilk::data::string& str)
	{
		lyramilk::data::string dst = iconv(str,"utf8//ignore","wchar_t");
		if(dst.size()&1){
			dst.push_back(0);
		}
		return lyramilk::data::wstring((wchar_t*)dst.c_str(),dst.size() >> lyramilk::data::intc<sizeof(wchar_t)>::square);
	}

	lyramilk::data::string inline unicode_utf8(const lyramilk::data::wstring& str)
	{
		lyramilk::data::string src((char*)str.c_str(),str.size() << lyramilk::data::intc<sizeof(wchar_t)>::square);
		lyramilk::data::string dst = iconv(src,"wchar_t","utf8");
		return dst;
	}


	class u2a
	{
		lyramilk::data::string p;
	  public:
		u2a(const lyramilk::data::wstring& wstr)
		{
			p = unicode_utf8(wstr);
		}
		
		operator lyramilk::data::string()
		{
			return p;
		}
	};

	class a2u
	{
		lyramilk::data::wstring p;
	  public:
		a2u(const lyramilk::data::string& str)
		{
			p = utf8_unicode(str);
		}
		
		operator lyramilk::data::wstring()
		{
			return p;
		}
	};

	#define t2a(x) (x)
	#define a2t(x) (x)

	#define t2u(x) a2u(x)
	#define u2t(x) u2a(x)
#endif

lyramilk::data::var::var()
{
	t = t_invalid;
}

lyramilk::data::var::var(const lyramilk::data::var& v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::~var()
{
	clear();
}

lyramilk::data::var::var(const unsigned char* v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(const char* v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(const wchar_t* v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(const lyramilk::data::chunk& v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(const lyramilk::data::string& v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(const lyramilk::data::wstring& v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(bool v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(int8 v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(uint8 v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(int16 v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(uint16 v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(int32 v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(uint32 v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(long v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(unsigned long v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(int64 v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(uint64 v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(double v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(float v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(const lyramilk::data::array& v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(const lyramilk::data::map& v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(const lyramilk::data::stringdict& v)
{
	t = t_invalid;
	assign(v);
}


lyramilk::data::var::var(const lyramilk::data::case_insensitive_unordered_map& v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(const lyramilk::data::case_insensitive_map& v)
{
	t = t_invalid;
	assign(v);
}

lyramilk::data::var::var(const lyramilk::data::datawrapper& v)
{
	t = t_invalid;
	assign(v);
}

bool lyramilk::data::var::operator ==(const lyramilk::data::var& v) const throw(lyramilk::data::type_invalid)
{
	switch(t){
	  case t_bin:{
			if(v.t == t_bool){
				return ((bool)*this) == v.u.b;
			}else{
				const lyramilk::data::chunk* bp = reinterpret_cast<const lyramilk::data::chunk*>(&u.bp);
				return bp->compare((lyramilk::data::chunk)v) == 0;
			}
		}break;
	  case t_str:{
			if(v.t == t_bool){
				return ((bool)*this) == v.u.b;
			}else{
				const lyramilk::data::string* bs = reinterpret_cast<const lyramilk::data::string*>(&u.bs);
				return bs->compare((lyramilk::data::string)v) == 0;
			}
		}break;
	  case t_wstr:{
			if(v.t == t_bool){
				return ((bool)*this) == v.u.b;
			}else{
				const lyramilk::data::wstring* bw = reinterpret_cast<const lyramilk::data::wstring*>(&u.bw);
				return bw->compare((lyramilk::data::wstring)v) == 0;
			}
		}break;
	  case t_bool:{
			return u.b == (bool)v;
		}break;
	  case t_int:{
			return u.i8 == (int64)v;
		}break;
	  case t_uint:{
			return u.u8 == (uint64)v;
		}break;
	  case t_double:{
			return u.f8 == (double)v;
		}break;
	  case t_array:{
			if(v.t != t_array){
				return false;
			}else{
				const lyramilk::data::array* ba = reinterpret_cast<const lyramilk::data::array*>(&u.ba);
				const lyramilk::data::array* vba = reinterpret_cast<const lyramilk::data::array*>(&v.u.ba);
				return *ba == *vba;
			}
		}break;
	  case t_map:{
			if(v.t != t_map){
				return false;
			}else{
				const lyramilk::data::map* bm = reinterpret_cast<const lyramilk::data::map*>(&u.bm);
				const lyramilk::data::map* vbm = reinterpret_cast<const lyramilk::data::map*>(&v.u.bm);
				if(bm->size() != vbm->size()) return false;
				for(lyramilk::data::map::const_iterator it1 = vbm->begin(),it2 = bm->begin();it1 != vbm->end() && it2 != bm->end();++it1,++it2){
					if(it1->first != it2->first || it1->second != it2->second) return false;
				}
				return true;
			}
		}break;
	  case t_user:{
			return false;
		}break;
	  case t_invalid:{
			return t_invalid == v.t;
		}break;
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator ==()",type_name(t).c_str()));
}

bool lyramilk::data::var::operator !=(const lyramilk::data::var& v) const throw(lyramilk::data::type_invalid)
{
	return !(*this == v);
}

bool lyramilk::data::var::operator <(const lyramilk::data::var& v) const throw(lyramilk::data::type_invalid)
{
	switch(t){
	  case t_bin:{
			const lyramilk::data::chunk* bp = reinterpret_cast<const lyramilk::data::chunk*>(&u.bp);
			return bp->compare((lyramilk::data::chunk)v) < 0;
		}break;
	  case t_str:{
			const lyramilk::data::string* bs = reinterpret_cast<const lyramilk::data::string*>(&u.bs);
			return bs->compare((lyramilk::data::string)v) < 0;
		}break;
	  case t_wstr:{
			const wstring* bw = reinterpret_cast<const wstring*>(&u.bw);
			return bw->compare((wstring)v) < 0;
		}break;
	  case t_array:{
			if(v.t != t_array){
				return t < v.t;
			}else{
				const array* ba = reinterpret_cast<const array*>(&u.ba);
				const array* vba = reinterpret_cast<const array*>(&v.u.ba);
				return *ba < *vba;
			}
		}break;
	  case t_map:{
			if(v.t != t_map){
				return t < v.t;
			}else{
				const map* bm = reinterpret_cast<const map*>(&u.bm);
				const map* vbm = reinterpret_cast<const map*>(&v.u.bm);
				if(bm->size() != vbm->size()) return false;
				for(map::const_iterator it1 = vbm->begin(),it2 = bm->begin();it1 != vbm->end() && it2 != bm->end();++it1,++it2){
					if(it2->first < it1->first) return false;
				}
				return bm->size() < vbm->size();
			}
		}break;
	  case t_bool:{
			return u.b < (bool)v;
		}break;
	  case t_int:{
			return u.i8 < (int64)v;
		}break;
	  case t_uint:{
			return u.u8 < (uint64)v;
		}break;
	  case t_double:{
			return u.f8 < (double)v;
		}break;
	  case t_invalid:{
			return false;
		}break;
	  default:
		throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator <()",type_name(t).c_str()));
	}
}

lyramilk::data::var& lyramilk::data::var::operator =(const lyramilk::data::var& v)
{
	return assign(v);
}

lyramilk::data::var& lyramilk::data::var::at(lyramilk::data::uint64 index) throw(lyramilk::data::type_invalid)
{
	if(t == t_array){
		array* ba = reinterpret_cast<array*>(&u.ba);
		return ba->at(index);
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的索引类型%s[%s]","lyramilk::data::var::at()",type_name(t).c_str(),"t_uint"));
}

lyramilk::data::var& lyramilk::data::var::at(const lyramilk::data::string& index) throw(lyramilk::data::type_invalid)
{
	if(t == t_map){
		map* bm = reinterpret_cast<map*>(&u.bm);
		return bm->operator[](index);
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的索引类型%s[%s]","lyramilk::data::var::at()",type_name(t).c_str(),"t_str"));
}

lyramilk::data::var& lyramilk::data::var::at(const wstring& index) throw(lyramilk::data::type_invalid)
{
	if(t == t_map){
		lyramilk::data::var str = index;
		map* bm = reinterpret_cast<map*>(&u.bm);
		return bm->operator[](str);
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的索引类型%s[%s]","lyramilk::data::var::at()",type_name(t).c_str(),"t_wstr"));
}

const lyramilk::data::var& lyramilk::data::var::at(lyramilk::data::uint64 index) const throw(lyramilk::data::type_invalid)
{
	if(t == t_array){
		const lyramilk::data::array* ba = reinterpret_cast<const lyramilk::data::array*>(&u.ba);
		return ba->at(index);
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的索引类型%s[%s]","lyramilk::data::var::at()",type_name(t).c_str(),"t_uint"));
}

const lyramilk::data::var& lyramilk::data::var::at(const lyramilk::data::string& index) const throw(lyramilk::data::type_invalid)
{
	if(t == t_map){
		const lyramilk::data::map* bm = reinterpret_cast<const lyramilk::data::map*>(&u.bm);
		map::const_iterator it = bm->find(index);
		if (it != bm->end()) {
			return it->second;
		}
		return lyramilk::data::var::nil;
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的索引类型%s[%s]","lyramilk::data::var::at()",type_name(t).c_str(),"t_str"));
}

const lyramilk::data::var& lyramilk::data::var::at(const wstring& index) const throw(lyramilk::data::type_invalid)
{
	if(t == t_map){
		lyramilk::data::var str = index;
		const map* bm = reinterpret_cast<const map*>(&u.bm);
		map::const_iterator it = bm->find(str.str());
		if (it != bm->end()) {
			return it->second;
		}
		return lyramilk::data::var::nil;
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的索引类型%s[%s]","lyramilk::data::var::at()",type_name(t).c_str(),"t_wstr"));
}

lyramilk::data::var& lyramilk::data::var::assign(const lyramilk::data::var& v)
{
	if(this == &v) return *this;
	vt newt = v.t;
	switch(v.t){
	  case t_bin:
		{
			const lyramilk::data::chunk* vbp = reinterpret_cast<const lyramilk::data::chunk*>(&v.u.bp);
			lyramilk::data::chunk cache = *vbp;
			clear();
			(new (u.bp) lyramilk::data::chunk)->swap(cache);
		}
		break;
	  case t_str:
		{
			const lyramilk::data::string* vbs = reinterpret_cast<const lyramilk::data::string*>(&v.u.bs);
			lyramilk::data::string cache = *vbs;
			clear();
			(new (u.bs) lyramilk::data::string)->swap(cache);
		}
		break;
	  case t_wstr:
		{
			const lyramilk::data::wstring* vbw = reinterpret_cast<const lyramilk::data::wstring*>(&v.u.bw);
			lyramilk::data::wstring cache = *vbw;
			clear();
			(new (u.bw) lyramilk::data::wstring)->swap(cache);
		}
		break;
	  case t_bool:
		{
			bool tmp = v.u.b;
			clear();
			u.b = tmp;
		}
		break;
	  case t_int:
		{
			int64 tmp = v.u.i8;
			clear();
			u.i8 = tmp;
		}
		break;
	  case t_uint:
		{
			uint64 tmp = v.u.u8;
			clear();
			u.u8 = tmp;
		}
		break;
	  case t_double:
		{
			double tmp = v.u.f8;
			clear();
			u.f8 = tmp;
		}
		break;
	  case t_array:
		{
			const lyramilk::data::array* vba = reinterpret_cast<const lyramilk::data::array*>(&v.u.ba);
			lyramilk::data::array cache = *vba;
			clear();
			(new (u.ba) lyramilk::data::array)->swap(cache);
		}
		break;
	  case t_map:
		{
			const lyramilk::data::map* vbm = reinterpret_cast<const lyramilk::data::map*>(&v.u.bm);
			lyramilk::data::map cache = *vbm;
			clear();
			(new (u.bm) lyramilk::data::map)->swap(cache);
		}
		break;
	  case t_user:
		{
			vu newu;
		  	newu.pu = v.u.pu->clone();
			clear();
			u = newu;
		}
		break;
	  case t_invalid:
		clear();
		break;
	  default:
		uint64 tu = v.u.u8;
		clear();
		u.u8 = tu;
	}
	t = newt;
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(const unsigned char* v)
{
	clear();
	t = t_str;
	new (u.bp) lyramilk::data::chunk(v?v:(const unsigned char*)"");
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(const char* v)
{
	clear();
	t = t_str;
	new (u.bs) lyramilk::data::string(v?v:"");
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(const wchar_t* v)
{
	clear();
	t = t_wstr;
	new (u.bw) lyramilk::data::wstring(v?v:L"");
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(const lyramilk::data::chunk& v)
{
	clear();
	t = t_bin;
	new (u.bp) lyramilk::data::chunk(v);
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(const string& v)
{
	clear();
	t = t_str;
	new (u.bs) lyramilk::data::string(v);
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(const wstring& v)
{
	clear();
	t = t_wstr;
	new (u.bw) lyramilk::data::wstring(v);
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(bool v)
{
	clear();
	t = t_bool;
	u.b = v;
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(int8 v)
{
	clear();
	t = t_int;
	u.i8 = v;
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(uint8 v)
{
	clear();
	t = t_uint;
	u.u8 = v;
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(int16 v)
{
	clear();
	t = t_int;
	u.i8 = v;
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(uint16 v)
{
	clear();
	t = t_uint;
	u.u8 = v;
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(int32 v)
{
	clear();
	t = t_int;
	u.i8 = v;
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(uint32 v)
{
	clear();
	t = t_uint;
	u.u8 = v;
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(long v)
{
	assign((lyramilk::data::intc<sizeof(long)>::t)v);
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(unsigned long v)
{
	assign((lyramilk::data::intc<sizeof(unsigned long)>::ut)v);
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(int64 v)
{
	clear();
	t = t_int;
	u.i8 = v;
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(uint64 v)
{
	clear();
	t = t_uint;
	u.u8 = v;
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(double v)
{
	clear();
	t = t_double;
	u.f8 = v;
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(float v)
{
	assign((double)v);
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(const lyramilk::data::array& v)
{
	clear();
	t = t_array;
	new (u.ba) lyramilk::data::array(v);
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(const lyramilk::data::map& v)
{
	clear();
	t = t_map;
	new (u.bm) lyramilk::data::map(v);
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(const lyramilk::data::stringdict& v)
{
	clear();
	t = t_map;
	lyramilk::data::map* bm = new (u.bm) lyramilk::data::map(v.bucket_count());
	stringdict::const_iterator it = v.begin();
	for(;it!=v.end();++it){
		bm->operator[](it->first) = it->second;
	}
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(const lyramilk::data::case_insensitive_unordered_map& v)
{
	clear();
	t = t_map;
	lyramilk::data::map* bm = new (u.bm) lyramilk::data::map(v.bucket_count());
	case_insensitive_unordered_map::const_iterator it = v.begin();
	for(;it!=v.end();++it){
		bm->operator[](it->first) = it->second;
	}
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(const lyramilk::data::case_insensitive_map& v)
{
	clear();
	t = t_map;
	lyramilk::data::map* bm = new (u.bm) lyramilk::data::map();
	case_insensitive_map::const_iterator it = v.begin();
	for(;it!=v.end();++it){
		bm->operator[](it->first) = it->second;
	}
	return *this;
}

lyramilk::data::var& lyramilk::data::var::assign(const lyramilk::data::datawrapper& v)
{
	clear();
	t = t_user;
	u.pu = v.clone();
	return *this;
}

/*lyramilk::data::var::operator lyramilk::data::chunk& () throw(lyramilk::data::type_invalid)
{
	if(t != t_bin) throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator lyramilk::data::chunk&()",type_name(t).c_str()));
	return *u.p;
}*/

lyramilk::data::var::operator lyramilk::data::chunk() const throw(lyramilk::data::type_invalid)
{
	switch(t){
	  case t_bin:{
			const lyramilk::data::chunk* bp = reinterpret_cast<const lyramilk::data::chunk*>(&u.bp);
			return *bp;
		}break;
	  case t_str:{
			const lyramilk::data::string* bs = reinterpret_cast<const string*>(&u.bs);
			return lyramilk::data::chunk((const unsigned char*)bs->c_str(),bs->size());
		}break;
	  case t_wstr:{
			const lyramilk::data::wstring* bw = reinterpret_cast<const wstring*>(&u.bw);
			return lyramilk::data::chunk((const unsigned char*)bw->c_str(),(bw->size() << (int)lyramilk::data::intc<sizeof(wchar_t)>::square));
		}break;
	  case t_bool:{
			return lyramilk::data::chunk((const unsigned char*)&u.b,sizeof(u.b));
		}break;
	  case t_int:{
			int64 i8 = reverse_order(u.i8);
			return lyramilk::data::chunk((const unsigned char*)&i8,sizeof(i8));
		}break;
	  case t_uint:{
			uint64 u8 = reverse_order(u.u8);
			return lyramilk::data::chunk((const unsigned char*)&u8,sizeof(u8));
		}break;
	  case t_double:{
			return lyramilk::data::chunk((const unsigned char*)&u.f8,sizeof(u.f8));
		}break;
	  case t_user:{
			return u.pu->get_bytes();
	    }break;
	  case t_array:
	  case t_map:
	  case t_invalid:{
			throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator lyramilk::data::chunk()",type_name(t).c_str()));
		}break;
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator lyramilk::data::chunk()",type_name(t).c_str()));
}

/*lyramilk::data::var::operator string& () throw(lyramilk::data::type_invalid)
{
	if(t != t_str) throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator string&()",type_name(t).c_str()));
	return *u.s;
}*/

lyramilk::data::var::operator string () const throw(lyramilk::data::type_invalid)
{
	switch(t){
	  case t_bin:{
			const lyramilk::data::chunk* bp = reinterpret_cast<const lyramilk::data::chunk*>(&u.bp);
			return string((const char*)bp->c_str(),bp->size());
		}break;
	  case t_str:{
			const string* bs = reinterpret_cast<const string*>(&u.bs);
			return *bs;
		}break;
	  case t_wstr:{
			const wstring* bw = reinterpret_cast<const wstring*>(&u.bw);
			return string(u2a(*bw));
		}break;
	  case t_bool:{
			return u.b?"true":"false";
		}break;
	  case t_int:{
			char buff[256];
			std::size_t fs = snprintf(buff,sizeof(buff),"%lld",u.i8);
			if(fs < 256) return buff;
			char* pbuff = (char*)calloc(1,fs + 1);
			snprintf(pbuff,fs+1,"%lld",u.i8);
			return pbuff;
		}break;
	  case t_uint:{
			char buff[256];
			std::size_t fs = snprintf(buff,sizeof(buff),"%llu",u.u8);
			if(fs < 256) return buff;
			char* pbuff = (char*)calloc(1,fs + 1);
			snprintf(pbuff,fs+1,"%llu",u.u8);
			return pbuff;
		}break;
	  case t_double:{
			char buff[256];
			std::size_t fs = snprintf(buff,sizeof(buff),"%f",u.f8);
			if(fs < 256) return buff;
			char* pbuff = (char*)calloc(1,fs + 1);
			snprintf(pbuff,fs+1,"%f",u.f8);
			return pbuff;
		}break;
	  case t_array:{
			const array* ba = reinterpret_cast<const array*>(&u.ba);
			array::const_iterator it = ba->begin();
			string str = "[";
			if(it != ba->end()){
				str += (string)*it;
				for(++it;it!=ba->end();++it){
					str += ",";
					str += (string)*it;
				}
			}
			str += "]";
			return str;
		}break;
	  case t_map:{
			const map* bm = reinterpret_cast<const map*>(&u.bm);

			string str = "{";
			map::const_iterator it = bm->begin();
			if(it != bm->end()){
				str += (string)it->first + ":" + (string)it->second;
				for(++it;it!=bm->end();++it){
					str += "," + (string)it->first + ":" + (string)it->second;
				}
			}
			str += "}";
			return str;
		}break;
	  case t_user:{
			return u.pu->get_str();
	    }break;
	  case t_invalid:{
			//throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator string()",type_name(t).c_str()));
			return "";
		}break;
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator string()",type_name(t).c_str()));
}

/*lyramilk::data::var::operator wstring& () throw(lyramilk::data::type_invalid)
{
	if(t != t_wstr) throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator wstring&()",type_name(t).c_str()));
	return *u.w;
}*/

lyramilk::data::var::operator wstring () const throw(lyramilk::data::type_invalid)
{
	switch(t){
	  case t_bin:{
			const lyramilk::data::chunk* bp = reinterpret_cast<const lyramilk::data::chunk*>(&u.bp);
			return wstring((const wchar_t*)bp->c_str(),(bp->size() << (int)lyramilk::data::intc<sizeof(wchar_t)>::square));
		}break;
	  case t_str:{
			const string* bs = reinterpret_cast<const string*>(&u.bs);
			return a2u(*bs);
		}break;
	  case t_wstr:{
			const wstring* bw = reinterpret_cast<const wstring*>(&u.bw);
			return *bw;
		}break;
	  case t_bool:{
			return u.b?L"true":L"false";
		}break;
	  case t_int:{
			wchar_t buff[256];
			swprintf(buff,sizeof(buff),L"%lld",u.i8);
			return buff;
		}break;
	  case t_uint:{
			wchar_t buff[256];
			swprintf(buff,sizeof(buff),L"%llu",u.u8);
			return buff;
		}break;
	  case t_double:{
			wchar_t buff[256];
			swprintf(buff,sizeof(buff),L"%f",u.f8);
			return buff;
		}break;
	  case t_array:{
			const array* ba = reinterpret_cast<const array*>(&u.ba);
			array::const_iterator it = ba->begin();
			wstring str = L"[";
			if(it != ba->end()){
				str += (wstring)*it;
				for(++it;it!=ba->end();++it){
					str += L",";
					str += (wstring)*it;
				}
			}
			str += L"]";
			return str;
		}break;
	  case t_map:{
			const map* bm = reinterpret_cast<const map*>(&u.bm);
			wstring str = L"{";
			map::const_iterator it = bm->begin();
			if(it != bm->end()){
				wstring strfirst = lyramilk::data::var(it->first);
				str += strfirst + L":" + (wstring)it->second;
				for(++it;it!=bm->end();++it){
					wstring strfirst = lyramilk::data::var(it->first);
					str += L"," + strfirst + L":" + (wstring)it->second;
				}
			}
			str += L"}";
			return str;
		}break;
	  case t_user:{
			return u.pu->get_wstr();
		}break;
	  case t_invalid:{
			return L"";
		}break;
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator wstring()",type_name(t).c_str()));
 }

lyramilk::data::var::operator bool () const throw(lyramilk::data::type_invalid)
{
	switch(t){
	  case t_bin:{
			const lyramilk::data::chunk* bp = reinterpret_cast<const lyramilk::data::chunk*>(&u.bp);
			if(sizeof(bool) > bp->size()) throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
			bool b;
			memcpy(&b,bp->c_str(),sizeof(b));
			return b;	}break;
	  case t_str:{
			const lyramilk::data::string* bs = reinterpret_cast<const lyramilk::data::string*>(&u.bs);
			lyramilk::data::string w = *bs;
			std::transform(bs->begin(),bs->end(),w.begin(),tolower);
			if(w.compare("true") == 0) return true;
			if(w.compare("false") == 0) return false;
			throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	  case t_wstr:{
			const lyramilk::data::wstring* bw = reinterpret_cast<const lyramilk::data::wstring*>(&u.bw);
			lyramilk::data::wstring w = *bw;
			std::transform(bw->begin(),bw->end(),w.begin(),towlower);
			if(w.compare(L"true") == 0) return true;
			if(w.compare(L"false") == 0) return false;
			throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	  case t_bool:
	  case t_int:
	  case t_uint:
	  case t_double:{
			return (int64)*this != 0;
		}break;
	  case t_array:{
			throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	  case t_map:{
			throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	  case t_user:{
			return u.pu->get_bool();
		}break;
	  case t_invalid:{
			throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
}

lyramilk::data::var::operator int8 () const throw(lyramilk::data::type_invalid)
{
	switch(t){
	  case t_bin:
	  case t_str:
	  case t_wstr:
	  case t_bool:
	  case t_int:
	  case t_uint:
	  case t_user:
	  case t_double:{
			return (int8)(int64)*this;
		}break;
	  case t_array:
	  case t_map:
	  case t_invalid:{
			throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int8()",type_name(t).c_str()));
		}break;
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int8()",type_name(t).c_str()));
}

lyramilk::data::var::operator uint8 () const throw(lyramilk::data::type_invalid)
{
	switch(t){
	  case t_bin:
	  case t_str:
	  case t_wstr:
	  case t_bool:
	  case t_int:
	  case t_uint:
	  case t_user:
	  case t_double:{
			return (uint8)(int64)*this;
		}break;
	  case t_array:
	  case t_map:
	  case t_invalid:{
			throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator uint8()",type_name(t).c_str()));
		}break;
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator uint8()",type_name(t).c_str()));
}

lyramilk::data::var::operator int16 () const throw(lyramilk::data::type_invalid)
{
	switch(t){
	  case t_bin:
	  case t_str:
	  case t_wstr:
	  case t_bool:
	  case t_int:
	  case t_uint:
	  case t_user:
	  case t_double:{
			return (int16)(int64)*this;
		}break;
	  case t_array:
	  case t_map:
	  case t_invalid:{
			throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int16()",type_name(t).c_str()));
		}
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int16()",type_name(t).c_str()));
}

lyramilk::data::var::operator uint16 () const throw(lyramilk::data::type_invalid)
{
	switch(t){
	  case t_bin:
	  case t_str:
	  case t_wstr:
	  case t_bool:
	  case t_int:
	  case t_uint:
	  case t_user:
	  case t_double:{
			return (uint16)(int64)*this;
		}break;
	  case t_array:
	  case t_map:
	  case t_invalid:{
			throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator uint16()",type_name(t).c_str()));
		}
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator uint16()",type_name(t).c_str()));
}

lyramilk::data::var::operator int32 () const throw(lyramilk::data::type_invalid)
{
	switch(t){
	  case t_bin:
	  case t_str:
	  case t_wstr:
	  case t_bool:
	  case t_int:
	  case t_uint:
	  case t_user:
	  case t_double:{
			return (int32)(int64)*this;
		}break;
	  case t_array:
	  case t_map:
	  case t_invalid:{
			throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int32()",type_name(t).c_str()));
		}
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int32()",type_name(t).c_str()));
}

lyramilk::data::var::operator uint32 () const throw(lyramilk::data::type_invalid)
{
	return (uint32)(int32)*this;
}

lyramilk::data::var::operator long () const throw(lyramilk::data::type_invalid)
{
	return (lyramilk::data::intc<sizeof(long)>::t)*this;
}

lyramilk::data::var::operator unsigned long () const throw(lyramilk::data::type_invalid)
{
	return (lyramilk::data::intc<sizeof(unsigned long)>::ut)*this;
}

lyramilk::data::var::operator int64 () const throw(lyramilk::data::type_invalid)
{
	switch(t){
	  case t_int:{
			return u.i8;
		}break;
	  case t_uint:{
			return u.u8;
		}break;
	  case t_bin:{
			const lyramilk::data::chunk* bp = reinterpret_cast<const lyramilk::data::chunk*>(&u.bp);
			if(sizeof(int64) > bp->size()) throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int64()",type_name(t).c_str()));
			int64 i8;
			memcpy(&i8,bp->c_str(),sizeof(i8));
			return i8;
		}break;
	  case t_str:{
			const string* bs = reinterpret_cast<const string*>(&u.bs);
			char* p;
			return strtoll(bs->c_str(),&p,10);
		}break;
	  case t_wstr:{
			const wstring* bw = reinterpret_cast<const wstring*>(&u.bw);
			wchar_t* p;
			return wcstoll(bw->c_str(),&p,10);
		}break;
	  case t_bool:{
			return u.b;
		}break;
	  case t_double:{
			return (int64)u.f8;
		}break;
	  case t_user:{
			return u.pu->get_int();
		}break;
	  case t_array:
	  case t_map:
	  case t_invalid:{
			throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int64()",type_name(t).c_str()));
		}break;
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int64()",type_name(t).c_str()));
}

lyramilk::data::var::operator uint64 () const throw(lyramilk::data::type_invalid)
{
	return (uint64)(int64)*this;
}


lyramilk::data::var::operator double () const throw(lyramilk::data::type_invalid)
{
	switch(t){
	  case t_bin:{
			const lyramilk::data::chunk* bp = reinterpret_cast<const lyramilk::data::chunk*>(&u.bp);
			if(sizeof(int64) > bp->size()) throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator uint64()",type_name(t).c_str()));
			double f8;
			memcpy(&f8,bp->c_str(),sizeof(f8));
			return f8;
		}break;
	  case t_str:{
			const string* bs = reinterpret_cast<const string*>(&u.bs);
			char* p;
			return strtod(bs->c_str(),&p);
		}break;
	  case t_wstr:{
			const wstring* bw = reinterpret_cast<const wstring*>(&u.bw);
			wchar_t* p;
			return wcstod(bw->c_str(),&p);
		}break;
	  case t_bool:{
			return u.b;
		}break;
	  case t_int:{
			return (double)u.i8;
		}break;
	  case t_uint:{
			return (double)u.u8;
		}break;
	  case t_double:{
			return u.f8;
		}break;
	  case t_user:{
			return u.pu->get_double();
		}break;
	  case t_array:
	  case t_map:
	  case t_invalid:{
			throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator double()",type_name(t).c_str()));
		}break;
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator double()",type_name(t).c_str()));
}

lyramilk::data::var::operator float () const throw(lyramilk::data::type_invalid)
{
	return (float)(double)*this;
}

lyramilk::data::var::operator lyramilk::data::var::array& () throw(lyramilk::data::type_invalid)
{
	array* ba = reinterpret_cast<array*>(&u.ba);
	if(t == t_array) return *ba;
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：%s类型无法转换为%s类型","lyramilk::data::var::operator lyramilk::data::var::array()",type_name(t).c_str(),type_name(t_array).c_str()));
}

lyramilk::data::var::operator const lyramilk::data::var::array& () const throw(lyramilk::data::type_invalid)
{
	const array* ba = reinterpret_cast<const array*>(&u.ba);
	if(t == t_array) return *ba;
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：%s类型无法转换为%s类型","lyramilk::data::var::operator lyramilk::data::var::array()",type_name(t).c_str(),type_name(t_array).c_str()));
}

lyramilk::data::var::operator lyramilk::data::var::map& ()  throw(lyramilk::data::type_invalid)
{
	map* bm = reinterpret_cast<map*>(&u.bm);
	if(t == t_map) return *bm;
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：%s类型无法转换为%s类型","lyramilk::data::var::operator lyramilk::data::var::map()",type_name(t).c_str(),type_name(t_map).c_str()));
}

lyramilk::data::var::operator const lyramilk::data::map& () const throw(lyramilk::data::type_invalid)
{
	const map* bm = reinterpret_cast<const map*>(&u.bm);
	if(t == t_map) return *bm;
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：%s类型无法转换为%s类型","lyramilk::data::var::operator lyramilk::data::var::map()",type_name(t).c_str(),type_name(t_map).c_str()));
}

lyramilk::data::chunk lyramilk::data::var::conv(const lyramilk::data::chunk& if_not_compat) const
{
	if(type_like(t_bin)) return *this;
	return if_not_compat;
}

lyramilk::data::string lyramilk::data::var::conv(const lyramilk::data::string& if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

lyramilk::data::wstring lyramilk::data::var::conv(const lyramilk::data::wstring& if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

lyramilk::data::string lyramilk::data::var::conv(const char* if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

lyramilk::data::string lyramilk::data::var::conv(char* if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

lyramilk::data::wstring lyramilk::data::var::conv(const wchar_t* if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

lyramilk::data::wstring lyramilk::data::var::conv(wchar_t* if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

lyramilk::data::chunk lyramilk::data::var::conv(const unsigned char* if_not_compat) const
{
	if(type_like(t_bin)) return *this;
	return if_not_compat;
}

lyramilk::data::chunk lyramilk::data::var::conv(unsigned char* if_not_compat) const
{
	if(type_like(t_bin)) return *this;
	return if_not_compat;
}

bool lyramilk::data::var::conv(bool if_not_compat) const
{
	if(type_like(t_bool)) return *this;
	return if_not_compat;
}

lyramilk::data::uint64 lyramilk::data::var::conv(lyramilk::data::int8 if_not_compat) const
{
	if(type_like(t_int)) return *this;
	return if_not_compat;
}
lyramilk::data::uint64 lyramilk::data::var::conv(lyramilk::data::uint8 if_not_compat) const
{
	if(type_like(t_int)) return *this;
	return if_not_compat;
}
lyramilk::data::uint64 lyramilk::data::var::conv(lyramilk::data::int16 if_not_compat) const
{
	if(type_like(t_int)) return *this;
	return if_not_compat;
}
lyramilk::data::uint64 lyramilk::data::var::conv(lyramilk::data::uint16 if_not_compat) const
{
	if(type_like(t_int)) return *this;
	return if_not_compat;
}
lyramilk::data::uint64 lyramilk::data::var::conv(lyramilk::data::int32 if_not_compat) const
{
	if(type_like(t_int)) return *this;
	return if_not_compat;
}
lyramilk::data::uint64 lyramilk::data::var::conv(lyramilk::data::uint32 if_not_compat) const
{
	if(type_like(t_int)) return *this;
	return if_not_compat;
}

lyramilk::data::uint64 lyramilk::data::var::conv(lyramilk::data::int64 if_not_compat) const
{
	if(type_like(t_int)) return *this;
	return if_not_compat;
}

lyramilk::data::uint64 lyramilk::data::var::conv(lyramilk::data::uint64 if_not_compat) const
{
	if(type_like(t_int)) return *this;
	return if_not_compat;
}

lyramilk::data::uint64 lyramilk::data::var::conv(long if_not_compat) const
{
	if(type_like(t_int)) return *this;
	return if_not_compat;
}

lyramilk::data::uint64 lyramilk::data::var::conv(unsigned long if_not_compat) const
{
	if(type_like(t_int)) return *this;
	return if_not_compat;
}

double lyramilk::data::var::conv(double if_not_compat) const
{
	if(type_like(t_double)) return *this;
	return if_not_compat;
}

lyramilk::data::var::array& lyramilk::data::var::conv(lyramilk::data::var::array& if_not_compat)
{
	if(!type_like(t_array)) return if_not_compat;
	return *this;
}

lyramilk::data::var::map& lyramilk::data::var::conv(lyramilk::data::var::map& if_not_compat)
{
	if(!type_like(t_map)) return if_not_compat;
	return *this;
}

const lyramilk::data::var::array& lyramilk::data::var::conv(const lyramilk::data::var::array& if_not_compat) const
{
	if(!type_like(t_array)) return if_not_compat;
	return *this;
}

const lyramilk::data::var::map& lyramilk::data::var::conv(const lyramilk::data::var::map& if_not_compat) const
{
	if(!type_like(t_map)) return if_not_compat;
	return *this;
}


lyramilk::data::datawrapper* lyramilk::data::var::userdata() const
{
	if(t != t_user)	return null;
	return u.pu;
}

lyramilk::data::var& lyramilk::data::var::operator[](const char* index) throw(lyramilk::data::type_invalid)
{
	return at(index);
}

lyramilk::data::var& lyramilk::data::var::operator[](const wchar_t* index) throw(lyramilk::data::type_invalid)
{
	return at(index);
}

lyramilk::data::var& lyramilk::data::var::operator[](const lyramilk::data::string& index) throw(lyramilk::data::type_invalid)
{
	return at(index);
}

lyramilk::data::var& lyramilk::data::var::operator[](const lyramilk::data::wstring& index) throw(lyramilk::data::type_invalid)
{
	return at(index);
}

lyramilk::data::var& lyramilk::data::var::operator[](lyramilk::data::uint64 index) throw(lyramilk::data::type_invalid)
{
	return at(index);
}

lyramilk::data::var& lyramilk::data::var::operator[](int index) throw(lyramilk::data::type_invalid)
{
	return at((lyramilk::data::uint64)index);
}

const lyramilk::data::var& lyramilk::data::var::operator[](const char* index) const throw(lyramilk::data::type_invalid)
{
	return at(index);
}

const lyramilk::data::var& lyramilk::data::var::operator[](const wchar_t* index) const throw(lyramilk::data::type_invalid)
{
	return at(index);
}

const lyramilk::data::var& lyramilk::data::var::operator[](const lyramilk::data::string& index) const throw(lyramilk::data::type_invalid)
{
	return at(index);
}

const lyramilk::data::var& lyramilk::data::var::operator[](const lyramilk::data::wstring& index) const throw(lyramilk::data::type_invalid)
{
	return at(index);
}

const lyramilk::data::var& lyramilk::data::var::operator[](lyramilk::data::uint64 index) const throw(lyramilk::data::type_invalid)
{
	return at(index);
}

const lyramilk::data::var& lyramilk::data::var::operator[](int index) const throw(lyramilk::data::type_invalid)
{
	return at((lyramilk::data::uint64)index);
}

lyramilk::data::var::vt lyramilk::data::var::type() const
{
	return t;
}

lyramilk::data::string lyramilk::data::var::type_name(lyramilk::data::var::vt nt)
{
	switch(nt){
	  case t_bin:
		return "t_bin";
	  case t_str:
		return "t_str";
	  case t_wstr:
		return "t_wstr";
	  case t_bool:
		return "t_bool";
	  case t_int:
		return "t_int";
	  case t_uint:
		return "t_uint";
	  case t_double:
		return "t_double";
	  case t_array:
		return "t_array";
	  case t_map:
		return "t_map";
	  case t_user:
		return "t_user";
	  case t_invalid:
		return "t_invalid";
	  default:
		return "t_unknow ";
	}
}

lyramilk::data::string lyramilk::data::var::type_name() const
{
	if(t == t_user) {
		return type_name(t) + "(" + u.pu->name() + ")";
	}
	return type_name(t);
}

lyramilk::data::var& lyramilk::data::var::type(lyramilk::data::var::vt nt) throw(lyramilk::data::type_invalid)
{
	if(t == nt) return *this;
	switch(nt){
	  case t_bin:{
			lyramilk::data::chunk t = *this;
			clear();
			new (u.bp) lyramilk::data::chunk(t);
		}break;
	  case t_str:{
			lyramilk::data::string t = *this;
			clear();
			new (u.bs) lyramilk::data::string(t);
		}break;
	  case t_wstr:{
			lyramilk::data::wstring t = *this;
			clear();
			new (u.bw) lyramilk::data::wstring(t);
		}break;
	  case t_bool:{
			bool t = *this;
			clear();
			u.b = t;
		}break;
	  case t_int:{
			lyramilk::data::int64 t = *this;
			clear();
			u.i8 = t;
		}break;
	  case t_uint:{
			lyramilk::data::uint64 t = *this;
			clear();
			u.u8 = t;
		}break;
	  case t_double:{
			double t = *this;
			clear();
			u.f8 = t;
		}break;
	  case t_array:{
			if(type_like(t_array)){
				array t = *this;
				clear();
				new (u.ba) lyramilk::data::array(t);
			}else{
				clear();
				new (u.ba) lyramilk::data::array();
			}
		}break;
	  case t_map:{
			if(type_like(t_map)){
				lyramilk::data::map t = *this;
				clear();
				new (u.bm) lyramilk::data::map(t);
			}else{
				clear();
				new (u.bm) lyramilk::data::map();
			}
		}break;
	  case t_user:{
			if(t != nt){
				clear();
			}
		}break;
	  case t_invalid:
		clear();
		break;
	  default:
		throw lyramilk::data::type_invalid(lyramilk::kdict("%s：错误的新类型%d","lyramilk::data::var::operator lyramilk::data::var::type()",nt));
	}
	t = nt;
	return *this;
}

bool lyramilk::data::var::type_like(lyramilk::data::var::vt nt) const
{
	if((nt == t_bin || nt == t_str || nt == t_wstr || nt == t_int ||  nt == t_uint ||  nt == t_double) && (t == t_bin || t == t_str || t == t_wstr || t == t_int ||  t == t_uint ||  t == t_double)){
		return true;
	}

	if(nt == t_user && t == t_user){
		return true;
	}

	if(nt == t_any) return true;
	if(nt == t_invalid) return false;
	if(nt == t_valid) return true;
	if(t == t_user) return u.pu->type_like(nt);
	if(nt == t) return true;
	return false;
}

lyramilk::data::var::array::size_type lyramilk::data::var::size() const throw(lyramilk::data::type_invalid)
{
	if(t == t_array){
		const lyramilk::data::array* ba = reinterpret_cast<const lyramilk::data::array*>(&u.ba);
		return ba->size();
	}
	if(t == t_map){
		const lyramilk::data::map* bm = reinterpret_cast<const lyramilk::data::map*>(&u.bm);
		return bm->size();
	}
	return 0;
}

lyramilk::data::string lyramilk::data::var::str() const
{
	return *this;
}

void lyramilk::data::var::clear()
{
	if(t == t_invalid){
		return;
	}

	switch(t){
	  case t_bool:
	  case t_int:
	  case t_uint:
	  case t_double:
	  case t_invalid:
		break;
	  case t_bin:
		{
			const lyramilk::data::chunk* bp = reinterpret_cast<const lyramilk::data::chunk*>(&u.bp);
			bp->~chunk();
		}
		break;
	  case t_str:
		{
			const lyramilk::data::string* bs = reinterpret_cast<const lyramilk::data::string*>(&u.bs);
			bs->~string();
		}
		break;
	  case t_wstr:
		{
			const lyramilk::data::wstring* bw = reinterpret_cast<const lyramilk::data::wstring*>(&u.bw);
			bw->~wstring();
		}
		break;
	  case t_array:
		{
			const lyramilk::data::array* ba = reinterpret_cast<const lyramilk::data::array*>(&u.ba);
			ba->~array();
		}
		break;
	  case t_map:
		{
			const lyramilk::data::map* bm = reinterpret_cast<const lyramilk::data::map*>(&u.bm);
			bm->~map();
		}
		break;
	  case t_user:
		{
			u.pu->destory();
		}
		break;
	}
	t = t_invalid;
}

/* 序列化相关 */

bool inline is_igendian()
{
	union{
		short s;
		char c[1];
	}u;
	u.s = 1;
	return u.c[0] == 0;
}

const bool g_igendian = is_igendian();

typedef unsigned short array_size_type;
typedef unsigned int string_size_type;

template <typename T>
void write(lyramilk::data::ostream& os,T& t)
{
	os.write((const char*)&t,sizeof(T));
}

template <typename T>
bool read(lyramilk::data::istream& is,T& t)
{
	is.read((char*)&t,sizeof(T));
	return is.gcount() == sizeof(T);
}

bool lyramilk::data::var::_serialize(ostream& os) const throw(lyramilk::data::type_invalid)
{
	if((t&0x7f) != t) throw lyramilk::data::type_invalid(lyramilk::kdict("%s：不支持的类型%d","lyramilk::data::var::serialize()",(t&0x7f)));
	unsigned char m;
	m = g_igendian<<7;
	m |= t&0x7f;

	switch(t){
	  case t_bin:{
			const lyramilk::data::chunk* bp = reinterpret_cast<const lyramilk::data::chunk*>(&u.bp);
			string_size_type size = (string_size_type)bp->size();
			write(os,m);
			write(os,size);
			os.write((const char*)bp->c_str(),size);
			return true;
		}break;
	  case t_str:{
			const lyramilk::data::string* bs = reinterpret_cast<const lyramilk::data::string*>(&u.bs);
			lyramilk::data::string utf8str = a2t(*bs);
			string_size_type size = (string_size_type)utf8str.size();
			write(os,m);
			write(os,size);
			os.write(utf8str.c_str(),size);
			return true;
		}break;
	  case t_wstr:{
			const lyramilk::data::wstring* bw = reinterpret_cast<const lyramilk::data::wstring*>(&u.bw);
			lyramilk::data::string utf8str = u2t(*bw);
			string_size_type size = (string_size_type)utf8str.size();
			write(os,m);
			write(os,size);
			os.write(utf8str.c_str(),size);
			return true;
		}break;
	  case t_bool:
	  case t_int:
	  case t_uint:{
			write(os,m);
			write(os,u.u8);
			return true;
		}break;
	  case t_double:{
			write(os,m);
			write(os,u.f8);
			return true;
		}break;
	  case t_array:{
			const lyramilk::data::array* ba = reinterpret_cast<const lyramilk::data::array*>(&u.ba);
			array_size_type size = (array_size_type)ba->size();
			write(os,m);
			write(os,size);
			array::const_iterator it = ba->begin();
			for(;it != ba->end();++it){
				if(!it->_serialize(os)) return false;
			}
			return true;
		}break;
	  case t_map:{
			const lyramilk::data::map* bm = reinterpret_cast<const lyramilk::data::map*>(&u.bm);
			array_size_type size = (array_size_type)bm->size();
			write(os,m);
			write(os,size);
			map::const_iterator it = bm->begin();
			for(;it != bm->end();++it){
				{
					string utf8str = a2t(it->first);
					string_size_type size = (string_size_type)utf8str.size();
					unsigned char mstr = (g_igendian<<7)|(t_str&0x7f);
					write(os,mstr);
					write(os,size);
					os.write((const char*)utf8str.c_str(),size);
				}
				if(!it->second._serialize(os)) return false;
			}
			return true;
		}break;
	  case t_user:
		return true;
	  case t_invalid:
		return false;
	}
	throw lyramilk::data::type_invalid(lyramilk::kdict("%s：不支持的类型%d","lyramilk::data::var::serialize()",(t&0x7f)));
}

bool lyramilk::data::var::_deserialize(istream& is)
{
	unsigned char m;
	if(!read(is,m)) return false;
	vt ts = (vt)(m&0x7f);
	bool r = (g_igendian?1:0) != (m>>7);

	switch(ts){
	  case t_bin:{
			string_size_type size = 0;
			if(!read(is,size)) return false;
			if(r) size = reverse_order(size);
			lyramilk::data::chunk binstr;
			binstr.resize(size);
			is.read((char*)binstr.c_str(),size);
			if(is.gcount() == size){
				assign(binstr);
				return true;
			}
			return false;
		}break;
	  case t_str:{
			string_size_type size = 0;
			if(!read(is,size)) return false;
			if(r) size = reverse_order(size);
			lyramilk::data::string utf8str;
			utf8str.resize(size);
			is.read((char*)utf8str.c_str(),size);
			if(is.gcount() == size){
				lyramilk::data::string localstr = t2a(utf8str);
				assign(localstr);
				return true;
			}
			return false;
		}break;
	  case t_wstr:{
			string_size_type size = 0;
			if(!read(is,size)) return false;
			if(r) size = reverse_order(size);
			lyramilk::data::string utf8str;
			utf8str.resize(size);
			is.read((char*)utf8str.c_str(),size);
			if(is.gcount() == size){
				lyramilk::data::string localstr = t2a(utf8str);
				assign(wstring(a2u(localstr)));
				return true;
			}
			return false;
		}break;
	  case t_bool:
	  case t_int:
	  case t_uint:{
			if(!read(is,u.u8)) return false;
			if(r) u.u8 = reverse_order(u.u8);
			t = ts;
			return true;
		}break;
	  case t_double:{
			if(!read(is,u.f8)) return false;
			t = ts;
			return true;
		}break;
	  case t_array:{
			array_size_type size = 0;
			if(!read(is,size)) return false;
			if(r) size = reverse_order(size);
			type(t_array);
			array* ba = reinterpret_cast<array*>(&u.ba);
			for(array_size_type i=0;i<size;++i){
				lyramilk::data::var d;
				if(!d._deserialize(is)){
					clear();
					return false;
				}
				ba->push_back(d);
			}
			t = ts;
			return true;
		}break;
	  case t_map:{
			array_size_type size = 0;
			if(!read(is,size)) return false;
			if(r) size = reverse_order(size);
			type(t_map);
			map* bm = reinterpret_cast<map*>(&u.bm);
			for(array_size_type i=0;i<size;++i){
				lyramilk::data::var key;
				lyramilk::data::var value;
				if(!(key._deserialize(is) && value._deserialize(is))){
					clear();
					return false;
				}
				bm->operator[](key) = value;
			}
			t = ts;
			return true;
		}break;
	  case t_user:
		return true;
	  case t_invalid:
		return false;
	}
	return false;
}

bool lyramilk::data::var::serialize(ostream& os) const throw(lyramilk::data::type_invalid)
{
	ostream::streamoff bpos = os.tellp();
	int32 size = 0;
	write(os,size);
	bool ret = _serialize(os);
	if(ret){
		ostream::streamoff epos = os.tellp();
		os.seekp(bpos,ostream::beg);
		size = (int32)(epos - bpos - sizeof(size));
		if(size > 0){
			size = htonl(size);
			write(os,size);
			os.seekp(epos,ostream::beg);
			if(os.good()) return true;
		}
	}
	os.clear();
	os.seekp(bpos,ostream::beg);
	return false;
}

bool lyramilk::data::var::deserialize(istream& is)
{
	istream::streamoff bpos = is.tellg();
	is.seekg(0,istream::end);
	istream::streamoff epos = is.tellg();
	is.seekg(bpos,istream::beg);

	int32 objsize = 0;
	if(read(is,objsize)){
		objsize = ntohl(objsize);
		int32 realsize = (int32)(epos - bpos - sizeof(objsize));
		if(objsize <= realsize){
			if(_deserialize(is)){
				return true;
			}
		}
	}

	is.clear();
	is.seekg(bpos,istream::beg);
	return false;
}

void lyramilk::data::var::dump(lyramilk::data::ostream& os) const
{
	os << str() << std::endl;
}

lyramilk::data::strings inline split(const lyramilk::data::string& data,const lyramilk::data::string& sep)
{
	lyramilk::data::strings lines;
	std::size_t posb = 0;
	do{
		std::size_t poscrlf = data.find(sep,posb);
		if(poscrlf == data.npos){
			lyramilk::data::string newline = data.substr(posb);
			posb = poscrlf;
			lines.push_back(newline);
		}else{
			lyramilk::data::string newline = data.substr(posb,poscrlf - posb);
			posb = poscrlf + sep.size();
			lines.push_back(newline);
		}
	}while(posb != data.npos);
	return lines;
}

lyramilk::data::strings inline pathof(const lyramilk::data::string& varpath) throw(lyramilk::data::type_invalid)
{
	lyramilk::data::strings ret;
	lyramilk::data::strings v = split(varpath,"/");
	lyramilk::data::strings::iterator it = v.begin();
	if(it==v.end()) return ret;
	while(it!=v.end()){
		if(it->compare(".") == 0 || it->empty()){
			++it;
			continue;
		}
		ret.push_back(*it);
		break;
	}
	for(++it;it!=v.end();++it){
		if(it->compare(".") == 0 || it->empty()) continue;
		if(it->compare("..") == 0 && !ret.empty()){
			ret.pop_back();
			continue;
		}
		ret.push_back(*it);
	}
	return ret;
}


lyramilk::data::var& lyramilk::data::var::path(const lyramilk::data::string& varpath) throw(lyramilk::data::type_invalid)
{
	lyramilk::data::strings fields = pathof(varpath);

	//如果前面的回退无法清除，说明想要的值越过了根，不允许。
	if(!fields.empty() && fields.at(0).compare("..") == 0){
		throw lyramilk::data::type_invalid(lyramilk::kdict("%s 路径错误，试图越过了根结点：%s","lyramilk::data::var::path()",varpath.c_str()));
	}

	//如果路径表达式为空或路径表达式只表达一个目录，则直接返回根。
	if(fields.size() == 0){
		return *this;
	}

	//此时的fields中包含且仅包含枝节点。
	lyramilk::data::strings::iterator it = fields.begin();
	//如果var是空的
	lyramilk::data::var* p = this;
	/*
	if(p->type() == t_invalid){
		p->type(t_map);
	}
	if((p->type() & (t_map | t_array)) == 0) throw lyramilk::data::type_invalid(lyramilk::kdict("%s 路径：根元素不是t_map或t_array(%s)","lyramilk::data::var::path()",varpath.c_str()));
*/
	for(;it!=fields.end();++it){
		if(p->type() == t_array){
			lyramilk::data::string& str = *it;
			if(str.find_first_not_of("0123456789") != str.npos){
				throw lyramilk::data::type_invalid(lyramilk::kdict("%s 路径：t_array类型只能接收纯整数的字符串形式(%s)","lyramilk::data::var::path()",varpath.c_str()));
			}
			unsigned int index = atoi(str.c_str());
			lyramilk::data::array* ba = reinterpret_cast<lyramilk::data::array*>(&p->u.ba);
			if(ba->size() == index){
				ba->push_back(nil);
			}else if(ba->size() < index + 1){
				throw lyramilk::data::type_invalid(lyramilk::kdict("%s 路径：t_array类型越界(%s)","lyramilk::data::var::path()",varpath.c_str()));
			}
			p = &ba->at(index);
		}else if(p->type() == t_map){
			lyramilk::data::map* bm = reinterpret_cast<lyramilk::data::map*>(&p->u.bm);
			p = &bm->operator[](*it);
		}else if(p->type() == t_invalid){
			lyramilk::data::string& str = *it;
			if(str.find_first_not_of("0123456789") == str.npos){
				unsigned int index = atoi(str.c_str());
				p->type(t_array);

				lyramilk::data::array* ba = reinterpret_cast<lyramilk::data::array*>(&p->u.ba);
				if(ba->size() == index){
					ba->push_back(nil);
					p = &ba->back();
				}else if(ba->size() < index - 1){
					p = &ba->operator[](index);
				}
			}else{
				p->type(t_map);
				lyramilk::data::map* bm = reinterpret_cast<lyramilk::data::map*>(&p->u.bm);
				p = &bm->operator[](str);
			}
		}else{
			throw lyramilk::data::type_invalid(lyramilk::kdict("%s 路径：%s","lyramilk::data::var::path()",varpath.c_str()));
		}
	}
	return *p;
}

const lyramilk::data::var& lyramilk::data::var::path(const lyramilk::data::string& varpath) const throw(lyramilk::data::type_invalid)
{
	lyramilk::data::strings fields = pathof(varpath);

	//如果前面的回退无法清除，说明想要的值越过了根，不允许。
	if(!fields.empty() && fields.at(0).compare("..") == 0){
		throw lyramilk::data::type_invalid(lyramilk::kdict("%s 路径错误，试图越过了根结点：%s","lyramilk::data::var::path()",varpath.c_str()));
	}

	//如果路径表达式为空或路径表达式只表达一个目录，则直接返回根。
	if(fields.size() == 0){
		return *this;
	}

	//此时的fields中包含且仅包含枝节点。
	lyramilk::data::strings::iterator it = fields.begin();
	//如果var是空的
	const lyramilk::data::var* p = this;
	/*
	if(p->type() == t_invalid){
		p->type(t_map);
	}
	if((p->type() & (t_map | t_array)) == 0) throw lyramilk::data::type_invalid(lyramilk::kdict("%s 路径：根元素不是t_map或t_array(%s)","lyramilk::data::var::path()",varpath.c_str()));
*/
	for(;it!=fields.end();++it){
		if(p->type() == t_array){
			lyramilk::data::string& str = *it;
			if(str.find_first_not_of("0123456789") != str.npos){
				throw lyramilk::data::type_invalid(lyramilk::kdict("%s 路径：t_array类型只能接收纯整数的字符串形式(%s)","lyramilk::data::var::path()",varpath.c_str()));
			}
			unsigned int index = atoi(str.c_str());
			const array* ba = reinterpret_cast<const array*>(&u.ba);
			if(ba->size() <= index){
				return lyramilk::data::var::nil;
			}
			p = &ba->at(index);
		}else if(p->type() == t_map){
			const map* bm = reinterpret_cast<const map*>(&u.bm);
			map::const_iterator it_map = bm->find(*it);
			if(it_map == bm->end()){
				return lyramilk::data::var::nil;
			}
			p = &it_map->second;
		}else{
			return lyramilk::data::var::nil;
		}
	}
	return *p;
}

template < >
lyramilk::data::chunk& lyramilk::data::var::as<lyramilk::data::chunk&>() throw(lyramilk::data::type_invalid)
{
	if(t != t_bin) throw lyramilk::data::type_invalid(lyramilk::kdict("as取引用时无法转换类型"));
	lyramilk::data::chunk* bp = reinterpret_cast<lyramilk::data::chunk*>(&u.bp);
	return *bp;
}

template < >
lyramilk::data::string& lyramilk::data::var::as<lyramilk::data::string&>() throw(lyramilk::data::type_invalid)
{
	if(t != t_str) throw lyramilk::data::type_invalid(lyramilk::kdict("as取引用时无法转换类型"));
	lyramilk::data::string* bs = reinterpret_cast<lyramilk::data::string*>(&u.bs);
	return *bs;
}

template < >
lyramilk::data::wstring& lyramilk::data::var::as<lyramilk::data::wstring&>() throw(lyramilk::data::type_invalid)
{
	if(t != t_wstr) throw lyramilk::data::type_invalid(lyramilk::kdict("as取引用时无法转换类型"));
	lyramilk::data::wstring* bw = reinterpret_cast<lyramilk::data::wstring*>(&u.bw);
	return *bw;
}

//}}	//namespace

std::ostream& operator << (std::ostream& os,const lyramilk::data::var& t)
{
	return os << t.str();
}

std::istream& operator >> (std::istream& is, lyramilk::data::var& t)
{
	char buff[4096];
	lyramilk::data::string str;
	while(is){
		is.read(buff,4096);
		str.append(buff,(unsigned int)is.gcount());
	}
	t = str;

	return is;
}
