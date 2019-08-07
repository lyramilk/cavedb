#ifndef _lyramilk_setproctitle_h_
#define _lyramilk_setproctitle_h_

namespace lyramilk{
	void init_setproctitle(int argc,const char** &argv);
	bool setproctitle(const char* p,...);
}

#endif
