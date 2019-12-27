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

cavedb_instance.slaveof_ssdb("192.168.226.168",7011,"CNVXlB8CmSZgDCB4yI8mqbquQmAr4XKt",b"",10233487807 + 1000000);
#cavedb_instance.slaveof_redis("127.0.0.1",6379,"",b"",0);


while True:
	time.sleep(1);
	print("cavedb speed %u/sec"%times);
	times = 0;