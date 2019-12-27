#ifndef _lyramilk_data_datawrapper_h_
#define _lyramilk_data_datawrapper_h_

#include "var.h"


/**
	@namespace lyramilk::data
	@brief 数据
	@details 该命名空间描述数据的表达形式。
*/

namespace lyramilk{namespace data
{
	class datawrapper
	{
	  public:
		virtual lyramilk::data::string name() const = 0;
		virtual lyramilk::data::datawrapper* clone() const = 0;
		virtual void destory() = 0;

		virtual bool type_like(lyramilk::data::var::vt nt) const = 0;

	  public:
		datawrapper();
		virtual ~datawrapper();
		//	t_str 必须
		virtual lyramilk::data::string get_str();
		//	t_array 必须
		virtual lyramilk::data::datawrapper& at(lyramilk::data::uint64 index);
		//	t_map 必须
		virtual lyramilk::data::datawrapper& at(const lyramilk::data::string& index);
		//	t_bin
		virtual lyramilk::data::chunk get_bytes();
		//	t_wstr
		virtual lyramilk::data::wstring get_wstr();
		//	t_bool
		virtual bool get_bool();
		//	t_int
		virtual lyramilk::data::int64 get_int();
		//	t_double
		virtual double get_double();
		//	t_map
		virtual lyramilk::data::datawrapper& at(const lyramilk::data::wstring& index);
	};


/*
	template <typename T>
	class pointer_datawrapper:public datawrapper
	{

	  public:
		T* ptr;
	  public:
		pointer_datawrapper(T* p);
	  	virtual ~pointer_datawrapper();

		virtual lyramilk::data::string name() const
		{
			return class_name();
		}

		virtual lyramilk::data::datawrapper* clone() const
		{
			return new pointer_datawrapper(ptr);
		}
		virtual void destory()
		{
			delete this;
		}
		virtual bool type_like(lyramilk::data::var::vt nt) const
		{
			return false;
		}
	};
*/


}}
#endif
