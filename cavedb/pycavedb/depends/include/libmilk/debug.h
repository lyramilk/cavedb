#ifndef _lyramilk_debug_debug_h_
#define _lyramilk_debug_debug_h_

#include <exception>
#include <execinfo.h>

#define LYRAMILK_STACK_TRACE(N) {void * a[N];int c = backtrace(a, N); char **s = backtrace_symbols(a, c);for (int i = 0; i < c; ++i) printf("%s\n",s[i]);free(s);}

namespace lyramilk{ namespace debug
{
}}
#endif

