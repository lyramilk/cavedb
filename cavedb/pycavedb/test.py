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
		if args[0]  != "hset":
			return True;
		if args[2]  != "rtimestamp":
			return True;
		print args
		return True;

cavedb_instance = cavedb_impl();


last_offset = 0;
last_replid = "";

cavedb_instance.slaveof_redis("127.0.0.1",6379,"",last_replid,last_offset);


while True:
	time.sleep(1);
	print("cavedb speed %u/sec"%times);
	times = 0;