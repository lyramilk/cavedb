# cavedb

这是一个实现了ssdb和redis同步协议的同步工具，它可以从ssdb或redis同步数据。
目前同步redis还有bug，从比较大的redis同步的时候会出现被主库中断连接的情况，所以实际使用中以ssdb为主。
同步不支持redis的list和ssdb的queue容器。遇到操作这个容器的命令请忽略。

##第一版本 完整实现redis语义
如有需要可以翻看git提交的历史记录。第一版本中实现了比较完整的语义支持redis但性能不佳，其中包括zset通过double类型分数排序而重写了leveldb的数据比较对象。


##第二版本 简洁，实用
整体结构有三大部分:
1.同步工具slave_ssdb和slave_redis用于走主从同步协议从ssdb或redis拉取数据
2.数据接收工具store，从slave_xxxx中同步来的数据会回调store存储工具，实现了这个接口就可以拿到数据了，需要注意的是同步线程是单线程的。
3.一系列包含存储功能的store的实现，随库自带的几个store走极简风格，只响应了ssdb的hashmap容器。并且在读的时候只支持hexist、hget、hgetall这三个功能。


#编译方法
依赖libmilk-devel这是我的另外一个工程

    cmake3 [cavedb目录]
    make

#打包rpm方法，需要rpmbuild

    cmake3 [cavedb目录]
    cd [cavedb目录]/pkg
    rpmbuild -bb cavedb.spec

#pycavedb示例，创建对象继承于cavedb.cavedb。调用其 slaveof_redis/slaveof_ssdb方法，从redis/ssdb同步数据，其中last_replid和last_offset表示同步进度，从覆盖的notify_psync/notify_command/notify_idle方法中可以获取同步进度

    #! /usr/bin/python
    #coding:utf-8
    
    
    import cavedb
    import time
    
    
    times = 0;
    
    class cavedb_impl(cavedb.cavedb):
    	def notify_command(self,replid,offset,args):
    		global times
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
