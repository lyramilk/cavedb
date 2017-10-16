#ifndef _casedb_leveldb_util_h_
#define _casedb_leveldb_util_h_

#include <leveldb/db.h>

namespace lyramilk{ namespace cave
{
	class leveldb_iterator
	{
		leveldb::Iterator* it;
	  public:
		leveldb_iterator(leveldb::Iterator* t)
		{
			it = t;
		}
		~leveldb_iterator()
		{
			delete it;
		}

		leveldb::Iterator* operator ->()
		{
			return it;
		}

		operator leveldb::Iterator*()
		{
			return it;
		}
	};
}}

#endif
