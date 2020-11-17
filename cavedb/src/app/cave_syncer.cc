#include <iostream>
#include <stdlib.h>
#include "store/stdmap_minimal.h"
#include "store/leveldb_minimal_adapter.h"
#include "store/sparse_hash_map_minimal.h"
#include "store/dense_hash_map_minimal.h"
#ifdef LMDB_FOUND
	#include "store/lmdb_minimal.h"
#endif
#include "store/rocksdb_minimal.h"
#include "slave_ssdb.h"
#include <signal.h>
#include <libmilk/dict.h>
#include <libmilk/log.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>

#include <sys/wait.h>

void useage(lyramilk::data::string selfname)
{
	std::cout << "useage:" << selfname << " [optional]" << std::endl;
	std::cout << "\t-d --daemon                                  \t" << "以服务方式启动" << std::endl;
	std::cout << "\t-h --ssdb-host         <ssdb Host>           \t" << "ssdb主库的host" << std::endl;
	std::cout << "\t-p --ssdb-port         <ssdb 端口>           \t" << "ssdb主库的port" << std::endl;
	std::cout << "\t-a --ssdb-password     <ssdb 密码>           \t" << "ssdb主库的密码" << std::endl;
	std::cout << "\t-e --leveldb-path      <leveldb 路径>        \t" << "leveldb路径" << std::endl;
	std::cout << "\t-? --help                                    \t显示这个帮助" << std::endl;
}

struct option long_options[] = {
	{ "daemon", required_argument, NULL, 'd' },
	{ "ssdb-host", required_argument, NULL, 'h' },
	{ "ssdb-port", required_argument, NULL, 'p' },
	{ "ssdb-password", required_argument, NULL, 'a' },
	{ "leveldb-path", required_argument, NULL, 'e' },
	{ "help", required_argument, NULL, '?' },
	{ 0, 0, 0, 0},
};

int main(int argc,char* argv[])
{
	bool isdaemon = false;
	lyramilk::data::string ssdb_host;
	lyramilk::data::uint16 ssdb_port = 0;
	lyramilk::data::string ssdb_password;
	lyramilk::data::string leveldb_path;

	lyramilk::data::string selfname = argv[0];
	if(argc > 1){
		int oc;
		while((oc = getopt_long(argc, argv, "dh:p:a:e:?",long_options,nullptr)) != -1){
			switch(oc)
			{
			  case 'd':
				isdaemon = true;
				break;
			  case 'h':
				ssdb_host = optarg;
				break;
			  case 'p':
				ssdb_port = atoi(optarg);
				break;
			  case 'a':
				ssdb_password = optarg;
				break;
			  case 'e':
				leveldb_path = optarg;
				break;
			  case '?':
			  default:
				useage(selfname);
				return 0;
			}
		}
	}else{
		useage(selfname);
		return 0;
	}
	if(isdaemon){
		daemon(0,0);
	}


	signal(SIGPIPE, SIG_IGN);

#if 1
	lyramilk::cave::rocksdb_minimal* mstore = lyramilk::cave::rocksdb_minimal::open(leveldb_path,1000,true);

	/*
	lyramilk::cave::leveldb_minimal_adapter mstore;
	mstore.open_leveldb(leveldb_path,1000);
	if(isneedcompact){
		mstore.compact();
	}
	*/
#else
	//lyramilk::cave::dense_hash_map_minimal mstore;
	//lyramilk::cave::sparse_hash_map_minimal mstore;
	lyramilk::cave::stdmap_minimal* mstore = new lyramilk::cave::stdmap_minimal;
#endif
	lyramilk::cave::slave_ssdb datasource;

	lyramilk::data::string replid = "";
	lyramilk::data::uint64 offset = 0;
	mstore->get_sync_info("",&replid,&offset);
	datasource.slaveof(ssdb_host,ssdb_port,ssdb_password,"",replid,offset,mstore);

	while(true){
		sleep(6);
	}
	return 0;
}