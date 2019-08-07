#ifndef _lyramilk_script_engine_h_
#define _lyramilk_script_engine_h_

#include "var.h"
#include "atom.h"
#include "exception.h"

/**
	@namespace lyramilk::script
	@brief 该命名空间用来封装脚本，对每种脚本提供统一的定义形式。
*/

namespace lyramilk{namespace script
{

	class _lyramilk_api_ sclass
	{
	  public:
		sclass();
	  	virtual ~sclass();

	  	virtual bool iterator_begin();
	  	virtual bool iterator_next(std::size_t idx,lyramilk::data::var* v);
	  	virtual void iterator_end();

	  	virtual bool set_property(const lyramilk::data::string& k,const lyramilk::data::var& v);
	  	virtual bool get_property(const lyramilk::data::string& k,lyramilk::data::var* v);
	};

	/**
		@brief 脚本引擎接口
	*/
	class _lyramilk_api_ engine
	{
	  protected:
		lyramilk::data::map userdata_0;
		lyramilk::data::map userdata_tmp;
	  public:

		/// 脚本向C++传递对象id时使用。
		static inline lyramilk::data::string s_user_objectid()
		{
			return "__script_object_id";
		}
		/// 脚本向C++传递对象指针时使用。
		static inline lyramilk::data::string s_user_nativeobject()
		{
			return "__script_native_object";
		}
		/// 脚本向C++传递函数id时使用。
		static inline lyramilk::data::string s_user_functionid()
		{
			return "__script_function_id";
		}

		/// 环境变量：脚本引擎对象指针。
		static inline lyramilk::data::string s_env_engine()
		{
			return "__engine";
		}

		/**
			@brief 函数指针：适配脚本可访问的C++对象中的函数
		*/
		typedef lyramilk::data::var (*functional_type_inclass)(const lyramilk::data::array& args,const lyramilk::data::map& env,void* nativeptr);
		typedef lyramilk::data::var (*functional_type)(const lyramilk::data::array& args,const lyramilk::data::map& env);

		/**
			@brief functional_type的map
		*/
		typedef std::map<lyramilk::data::string,functional_type_inclass,std::less<lyramilk::data::string>,lyramilk::data::allocator<lyramilk::data::string> > functional_map;

		/**
			@brief 函数指针：创建脚本可访问的C++对象
		*/
		typedef sclass* (*class_builder)(const lyramilk::data::array& args);

		/**
			@brief 函数指针：销毁脚本可访问的C++对象
		*/
		typedef void (*class_destoryer)(sclass*);

		engine();
		virtual ~engine();

		virtual lyramilk::data::var& get_userdata(const lyramilk::data::string& k);
		virtual bool set_userdata(const lyramilk::data::string& k,const lyramilk::data::var& v,bool temporary);
		/**
			@brief 从一个字符串中加载脚本代码
			@param script 字符串形式的脚本代码
			@return 返回true表示成功
		*/
		virtual bool load_string(const lyramilk::data::string& script) = 0;

		/**
			@brief 从一个文件中加载脚本代码
			@param scriptfile 脚本文件路径
			@return 返回true表示成功
		*/
		virtual bool load_file(const lyramilk::data::string& scriptfile);

		/**
			@brief 加载模块
			@param modulefile 脚本文件路径
			@return 返回true表示成功
		*/
		virtual bool load_module(const lyramilk::data::string& modulefile) = 0;

		/**
			@brief 执行脚本函数。
			@param func 脚本函数名
			@return 脚本的返回值
		*/
		virtual lyramilk::data::var call(lyramilk::data::var func);

		/**
			@brief 执行脚本函数。
			@param func 脚本函数名
			@param args 参数
			@return 脚本的返回值
		*/
		virtual lyramilk::data::var call(const lyramilk::data::var& func,const lyramilk::data::array& args) = 0;

		/**
			@brief 重置脚本引擎。
		*/
		virtual void reset();

		/**
			@brief 将一个脚本可访问的C++对象注入到脚本引擎中。
			@param classname 脚本中对象名字
			@param m 对象的成员函数表
			@param builder 该对象的创建函数
			@param destoryer 该对象的销毁函数
		*/
		virtual void define(const lyramilk::data::string& classname,functional_map m,class_builder builder,class_destoryer destoryer) = 0;

		/**
			@brief 将一个脚本可访问的C++函数注入到脚本引擎中。
			@param funcname 函数名
			@param func 脚本可访问的C++本地函数
		*/
		virtual void define(const lyramilk::data::string& funcname,functional_type func) = 0;

		/**
			@brief 将一个全局常量属性注入到脚本引擎中。
			@param key 函数名
			@param value 函数值
		*/
		virtual void define_const(const lyramilk::data::string& key,const lyramilk::data::var& value) = 0;

		/**
			@brief 将一个脚本可访问的C++对象装载到一个var对象中以作为参数由C++传递给脚本引擎。
			@param classname 脚本中对象名字
			@param args 参数
			@return 装载了脚本对象的var对象，该var对象只能传递给创建该对象的脚本引擎对象。
		*/
		virtual lyramilk::data::var createobject(const lyramilk::data::string& classname,const lyramilk::data::array& args) = 0;

		/**
			@brief 对于支持垃圾回收的脚本引擎，主动促使其进行垃圾回收。
		*/
		virtual void gc();

		/**
			@brief 返回脚本引擎的名字
		*/
		virtual lyramilk::data::string name() = 0;

		/**
			@brief 返回脚本当前脚本的文件名
		*/
		virtual lyramilk::data::string filename() = 0;

		/**
			@brief 该模板用于适配C++可访问的对象的成员函数到脚本引擎支持的形式。
			@details 该模板用于适配C++可访问的对象的成员函数到脚本引擎支持的形式。举例，如果number是一个C++类，而number的普通成员函数add函数符合functional_type形式，那么 lyramilk::script::engine::functional<number,&number::add>可以将该函数适配到非成员的functional_type形式。
		*/
		template <typename T,lyramilk::data::var (T::*Q)(const lyramilk::data::array& ,const lyramilk::data::map&)>
		static inline lyramilk::data::var functional(const lyramilk::data::array& args,const lyramilk::data::map& env,void* nativeptr)
		{
			T* pthis = (T*)nativeptr;
			return (pthis->*(Q))(args,env);
		}

		/**
			@brief 定义一个脚本引擎与其创建函数的对应关系。
			@param scriptname 脚本引擎的名字
			@param builder 脚本引擎对象的创建函数
		*/
		static bool define(const lyramilk::data::string& scriptname,lyramilk::script::engine* (*builder)(),void (*destoryer)(lyramilk::script::engine*));

		/**
			@brief 取消一个脚本引擎与其创建函数的对应关系。
			@param scriptname 脚本引擎的名字
		*/
		static void undef(const lyramilk::data::string& funcname);

		/**
			@brief 通过脚本引擎的名字创建一个脚本引擎对象。
			@param scriptname 脚本引擎的名字
		*/
		static lyramilk::script::engine* createinstance(const lyramilk::data::string& scriptname);

		/**
			@brief 销毁由createinstance创建的引擎对象。
		*/
		static void destoryinstance(const lyramilk::data::string& scriptname,lyramilk::script::engine* eng);
	};

	class _lyramilk_api_ engines : public lyramilk::threading::rentlist<lyramilk::script::engine>
	{
	  public:
		engines();
		virtual ~engines();

		virtual engine* underflow() = 0;
		virtual void onfire(engine* o);
		virtual void reset();
	};

	#define MILK_CHECK_SCRIPT_ARGS_LOG(log,lt,m,params,i,t)  {	\
		if(params.size() < i + 1){	\
			lyramilk::data::string str = D("参数太少");	\
			log(lt,m) << str << std::endl;	\
			throw lyramilk::exception(str);	\
		}	\
		const lyramilk::data::var& v = params.at(i);	\
		if(!v.type_like(t)){	\
			lyramilk::data::string str = D("参数%d类型不兼容:%s，期待%s",i+1,v.type_name().c_str(),lyramilk::data::var::type_name(t).c_str());	\
			log(lt,m) << str << std::endl;	\
			throw lyramilk::exception(str);	\
		}	\
	}

	#define MILK_CHECK_SCRIPT_ARGS(params,i,t)  {	\
		if(params.size() < i + 1){	\
			throw lyramilk::exception(D("参数太少"));	\
		}	\
		const lyramilk::data::var& v = params.at(i);	\
		if(!v.type_like(t)){	\
			throw lyramilk::exception(D("参数%d类型不兼容:%s，期待%s",i+1,v.type_name().c_str(),lyramilk::data::var::type_name(t).c_str()));	\
		}	\
	}



}}

#endif
