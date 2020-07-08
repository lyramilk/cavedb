import sys
#from distutils.core import setup, Extension
from setuptools import setup,Extension
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

with open("README.md", "r") as fh:
	long_description = fh.read()

setup (name = 'pycavedb',
	version = '1.2',
	description = 'cavedb-python',
	long_description=long_description,
	long_description_content_type="text/markdown",
	author = "lyramilk",
	ext_modules = [module1],
	author_email='lyramilk@qq.com',
	license='Apache License 2.0',
	url='https://github.com/lyramilk/cavedb', 
	classifiers=[
		"Intended Audience :: Developers",
		"Operating System :: OS Independent",
		"Natural Language :: Chinese (Simplified)",
		'Programming Language :: Python',
		'Programming Language :: Python :: 2',
		'Programming Language :: Python :: 2.5',
		'Programming Language :: Python :: 2.6',
		'Programming Language :: Python :: 2.7',
		'Programming Language :: Python :: 3',
		'Programming Language :: Python :: 3.2',
		'Programming Language :: Python :: 3.3',
		'Programming Language :: Python :: 3.4',
		'Programming Language :: Python :: 3.5',
		'Topic :: Utilities'
	],
	keywords = 'cavedb,ssdb,redis',
)