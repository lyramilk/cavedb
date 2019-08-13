#! /usr/bin/python
#coding:utf-8


import cavedb
import leveldb
import time


times = 0;

class cavedb_impl(cavedb.cavedb):
	def notify_command(self,replid,offset,args):
		global times
		#print(args)
		times += 1;
		return True;

cavedb_instance = cavedb_impl();

cavedb_instance.slaveof_ssdb("127.0.0.1",8888,"",b"",0);
#cavedb_instance.slaveof_redis("127.0.0.1",6379,"",b"",0);


while True:
	time.sleep(1);
	print("cavedb speed %u/sec"%times);
	times = 0;