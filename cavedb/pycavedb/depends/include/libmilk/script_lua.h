#include "config.h"
#if (defined LUAJIT_FOUND) && (!defined _lyramilk_script_lua_engine_h_)
#define _lyramilk_script_lua_engine_h_

#include "scriptengine.h"
#include <luajit-2.0/lua.hpp>

/**
	@namespace lyramilk::script::lua
	@brief 该命名空间用来封装脚本，但并不要求对每种脚本提供统一的定义形式。
*/

namespace lyramilk{namespace script{namespace lua
{
	class script_lua : public lyramilk::script::engine
	{
	  protected:
		lua_State *L;
		lua_State *L_template;
		lyramilk::data::string scriptfilename;
	  public:
		struct metainfo
		{
			class_builder ctr;
			class_destoryer dtr;
			void* self;
			lyramilk::script::engine *env;
			lyramilk::data::string name;
		};
	  public:
		script_lua();
		virtual ~script_lua();
		virtual bool load_string(const lyramilk::data::string& script);
		virtual bool load_file(const lyramilk::data::string& scriptfile);
		virtual bool load_module(const lyramilk::data::string& modulefile);

		virtual lyramilk::data::var call(const lyramilk::data::var& func,const lyramilk::data::array& args);
		virtual void define(const lyramilk::data::string& classname,functional_map m,class_builder builder,class_destoryer destoryer);
		virtual void define(const lyramilk::data::string& funcname,functional_type func);
		virtual void define_const(const lyramilk::data::string& key,const lyramilk::data::var& value);
		virtual lyramilk::data::var createobject(const lyramilk::data::string& classname,const lyramilk::data::array& args);
		virtual void reset();
		virtual void gc();
		virtual lyramilk::data::string name();
		virtual lyramilk::data::string filename();
	  private:
		bool init();
		std::map<lyramilk::data::string,metainfo> minfo;
		void clear();
	};

}}}

#endif
