import sys
from distutils.core import setup, Extension
version = sys.version_info

module1 = None;



other_sources = [
	"depends/libmilk/def.cpp",
	"depends/libmilk/dict.cpp",
	"depends/libmilk/log.cpp",
	"depends/libmilk/thread.cpp",
	"depends/libmilk/netio.cpp",
	"depends/libmilk/netaio.cpp",
	"depends/libmilk/aio.cpp",
	"depends/libmilk/testing.cpp",
	"depends/libmilk/var.cpp",
	"depends/libmilk/json.cpp",
	"depends/libmilk/codes.cpp",
	"depends/libmilk/datawrapper.cpp",
	"depends/libmilk/exception.cpp",
	"depends/cavedb/slave_redis.cpp",
	"depends/cavedb/slave_ssdb.cpp",
	"depends/cavedb/rdb.cpp",
	"depends/cavedb/redis_assert.c",
	"depends/cavedb/redis/endianconv.c",
	"depends/cavedb/redis/intset.c",
	"depends/cavedb/redis/lzf_c.c",
	"depends/cavedb/redis/lzf_d.c",
	"depends/cavedb/redis/quicklist.c",
	"depends/cavedb/redis/sds.c",
	"depends/cavedb/redis/sha1.c",
	"depends/cavedb/redis/util.c",
	"depends/cavedb/redis/ziplist.c",
	"depends/cavedb/redis/zipmap.c",
	"depends/cavedb/redis/zmalloc.c",
]

	
if version >= (2,0) and version < (3,0):
	module1 = Extension('cavedb',sources = ['cavedb_python_mutiplethread.cpp'] + other_sources,libraries = ['rt'],include_dirs=['./depends'],extra_compile_args=["-std=c99"])
elif version >= (3,0) and version < (4,0):
	module1 = Extension('cavedb',sources = ['cavedb_python_mutiplethread.cpp'] + other_sources,libraries = ['rt'],include_dirs=['./depends'],extra_compile_args=["-std=c99"])


setup (name = 'cavedb',
       version = '1.0',
       description = 'cavedb-python',
	   author = "lyramilk",
       ext_modules = [module1]
)