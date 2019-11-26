import sys
from distutils.core import setup, Extension
version = sys.version_info

module1 = None;
	
if version >= (2,0) and version < (3,0):
	module1 = Extension('cavedb',sources = ['cavedb_python_mutiplethread.cpp'],library_dirs=['./depends/lib64'],libraries = ['cavedb','milk'],include_dirs=['./depends/include'])
elif version >= (3,0) and version < (4,0):
	module1 = Extension('cavedb',sources = ['cavedb_python_mutiplethread.cpp'],library_dirs=['./depends/lib64'],libraries = ['cavedb','milk'],include_dirs=['./depends/include'])


setup (name = 'cavedb',
       version = '1.0',
       description = 'cavedb-python',
	   author = "lyramilk",
       ext_modules = [module1]
)