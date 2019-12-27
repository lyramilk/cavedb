#include "testing.h"
#include "dict.h"
#include <sstream>
#ifdef WIN32
#include <windows.h>
#include <time.h>
#endif
#ifdef __GNUC__
	#define D(x...) lyramilk::kdict(x)
#elif defined _MSC_VER
	#define D(...) lyramilk::kdict(__VA_ARGS__)
#endif

namespace lyramilk{ namespace debug
{

	void cpuinfo::bind(int cpucore)
	{
		cpu_set_t mask;
		CPU_SET(cpucore, &mask);
		pthread_setaffinity_np(pthread_self(),sizeof(mask),&mask);
	}

	int cpuinfo::count()
	{
		return get_nprocs();
	}

	void nsecdiff::mark()
	{
		clock_gettime(CLOCK_MONOTONIC_RAW, &timestamp);
	}

	long long nsecdiff::diff()
	{
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
		long long s = ts.tv_sec - timestamp.tv_sec;
		long long n = ts.tv_nsec - timestamp.tv_nsec;
		return s * 1000000000 + n;
	}

	clocktester::clocktester(timediff& _td):td(_td),outer(std::cout)
	{
		printable = true;
		td.mark();
	}
	clocktester::clocktester(timediff& _td,const lyramilk::data::string& msg):td(_td),outer(std::cout)
	{
		printable = true;
		str = msg;
		td.mark();
	}
	clocktester::clocktester(timediff& _td,std::ostream& os,const lyramilk::data::string& msg):td(_td),outer(os)
	{
		printable = true;
		str = msg;
		td.mark();
	}
	clocktester::~clocktester()
	{
		if(printable){
			long long des = td.diff();
			outer << str << D("耗时：%lld(纳秒)",des) << std::endl;
		}
	}

	void clocktester::cancel()
	{
		printable = false;
	}

	void clocktester::resume()
	{
		printable = true;
	}

	void clocktester::setmsg(const lyramilk::data::string& msg)
	{
		str = msg;
	}
	timer::timer()
	{
		l = v = time(0);
	}
	timer::~timer()
	{
	}
#ifdef WIN32
	timer::operator bool(){
		v = time(0);
		if(l != v && ((v % 1) == 0)){
			if (InterlockedCompareExchange((volatile unsigned long long*)&l, l, v)){
				return true;
			}
		}
		return false;
	}
#elif defined __GNUC__
	timer::operator bool(){
		v = time(0);
		if (l != v && ((v % 1) == 0)){
			if (__sync_bool_compare_and_swap(&l, l, v)){
				return true;
			}
		}
		return false;
	}
#endif

}}