#cavedb
这是一个实现了ssdb和redis同步协议的同步工具，它可以从ssdb或redis同步数据。

#pycavedb示例
1.创建对象继承于cavedb.cavedb。
2.调用其 slaveof_redis/slaveof_ssdb方法，从redis/ssdb同步数据，其中last_replid和last_offset表示同步进度，从覆盖的notify_psync/notify_command/notify_idle方法中可以获取同步进度。
3.在同步时，cavedb会插入几个特殊的同步命令，实际使用时需过滤掉它们，这几个命令分别为

| 命令  | 含义  |
| ------------ | ------------ |
| sync_start  | 开始同步  |
| sync_continue  |己同步完全量部分，开始同步增量部分   |



    #! /usr/bin/python
    #coding:utf-8

    import cavedb
    import time

    class cavedb_impl(cavedb.cavedb):
    	def notify_psync(self,replid,offset):
			return True;

    	def notify_idle(self,replid,offset):
			return True;

    	def notify_command(self,replid,offset,args):
    		print args
    		if len(args) > 0 and args[0] in ["sync_start","sync_overflow","sync_continue"]:
    			return True;
    		return True;

    cavedb_instance = cavedb_impl();


    last_offset = 0;
    last_replid = "";

    cavedb_instance.slaveof_redis("127.0.0.1",6379,"",last_replid,last_offset);

    while True:
    	time.sleep(1);
    	print("cavedb running...");
