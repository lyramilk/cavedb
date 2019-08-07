#ifndef _lyramilk_data_inotify_
#define _lyramilk_data_inotify_

#include "var.h"
#include "aio.h"
#include <sys/inotify.h>

namespace lyramilk{namespace data
{
	class inotify:public lyramilk::io::aioselector
	{
		lyramilk::io::native_filedescriptor_type fd;
		std::map<lyramilk::io::native_filedescriptor_type,lyramilk::data::string> wm;
		virtual bool notify_in();
		virtual bool notify_out();
		virtual bool notify_hup();
		virtual bool notify_err();
		virtual bool notify_pri();
		virtual lyramilk::io::native_filedescriptor_type getfd();
		virtual void ondestory();
		virtual void notify_event(inotify_event* ie);
	  protected:
		virtual void notify_add(const lyramilk::data::string& dirname,const lyramilk::data::string& filename);
		virtual void notify_modify(const lyramilk::data::string& dirname,const lyramilk::data::string& filename);
		virtual void notify_remove(const lyramilk::data::string& dirname,const lyramilk::data::string& filename);

		virtual void notify_other(inotify_event* ie);
	  public:
		static int flag();
		inotify();
		virtual ~inotify();
	
		lyramilk::io::native_filedescriptor_type add(const lyramilk::data::string& pathdirname);
		lyramilk::io::native_filedescriptor_type add(const lyramilk::data::string& filename,lyramilk::io::uint32 inotify_mask);
		void remove(lyramilk::io::native_filedescriptor_type childfd);
	};


	class inotify_file
	{
		lyramilk::io::native_filedescriptor_type fd;
		lyramilk::io::native_filedescriptor_type wfd;
		lyramilk::data::string filename;
		lyramilk::data::string dirname;
	  public:
		inotify_file(const lyramilk::data::string& pathdirname);
		virtual ~inotify_file();

		enum status
		{
			s_keep,
			s_modify,
			s_remove,
			s_add,
		};
		status check();
	};
}}
#endif
