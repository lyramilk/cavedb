import os;
import json;

def MkdirP(dirpath):
	if not os.path.exists(dirpath):
		os.makedirs(dirpath);
		print("创建了目录:" + dirpath);
	else:
		print("目录已存在:" + dirpath);

def PutConfigFile(filepath,data):
	if not os.path.exists(filepath):
		with open(filepath,"w") as f:
			f.write(data);
		print("创建了文件:" + filepath);
	else:
		print("文件已存在:" + filepath);

def CreateKvrocks(name,ip,port,password,masterIp = None):
	dbname = name;

	if not dbname.startswith("kvrocks-"):
		dbname = "kvrocks-" + dbname;
	if not dbname.endswith("-%d"%port):
		dbname = dbname + "-%d"%port;

	dbconfigbase = f'''
bind {ip} 127.0.0.1
port {port}
timeout 0
workers 8
daemonize yes
cluster-enabled no
maxclients 10000
requirepass {password}
db-name {dbname}.db
dir /data/kvrocks/{dbname}/data
log-dir /data/kvrocks/{dbname}/logs
pidfile /data/kvrocks/{dbname}/kvrocks.pid
slave-read-only yes
slave-priority 100
tcp-backlog 511
master-use-repl-port no
slave-serve-stale-data yes
slave-empty-db-before-fullsync no
purge-backup-on-fullsync no
max-replication-mb 0
max-io-mb 500
max-db-size 0
max-backup-to-keep 1
max-backup-keep-hours 24
max-bitmap-to-string-mb 16
slowlog-log-slower-than 100000
slowlog-max-len 128
supervised no
profiling-sample-ratio 0
profiling-sample-record-max-len 256
profiling-sample-record-threshold-ms 100
compaction-checker-range 0-7
auto-resize-block-and-sst yes
migrate-speed 4096
migrate-pipeline-size 16
migrate-sequence-gap 10000
rocksdb.metadata_block_cache_size 2048
rocksdb.subkey_block_cache_size 4096
rocksdb.share_metadata_and_subkey_block_cache yes
rocksdb.row_cache_size 0
rocksdb.max_open_files 8096
rocksdb.write_buffer_size 64
rocksdb.target_file_size_base 128
rocksdb.max_write_buffer_number 4
rocksdb.max_background_compactions 4
rocksdb.max_background_flushes 4
rocksdb.max_sub_compactions 2
rocksdb.max_total_wal_size 512
rocksdb.wal_ttl_seconds 10800
rocksdb.wal_size_limit_mb 16389
rocksdb.block_size 16389
rocksdb.cache_index_and_filter_blocks yes
rocksdb.compression snappy
rocksdb.compaction_readahead_size 2097152
rocksdb.delayed_write_rate 0
rocksdb.enable_pipelined_write no
rocksdb.level0_slowdown_writes_trigger 20
rocksdb.level0_stop_writes_trigger 40
rocksdb.level0_file_num_compaction_trigger 4
rocksdb.stats_dump_period_sec 0
rocksdb.disable_auto_compactions no
rocksdb.enable_blob_files no
rocksdb.min_blob_size 4096
rocksdb.blob_file_size 256
rocksdb.enable_blob_garbage_collection yes
rocksdb.blob_garbage_collection_age_cutoff 25
rocksdb.level_compaction_dynamic_level_bytes no
rocksdb.max_bytes_for_level_base 256
rocksdb.max_bytes_for_level_multiplier 10
backup-dir /data/kvrocks/{dbname}/data/backup
fullsync-recv-file-delay 0
'''


	MkdirP(f"/data/kvrocks/{dbname}/conf");
	MkdirP(f"/data/kvrocks/{dbname}/data");
	MkdirP(f"/data/kvrocks/{dbname}/logs");
	MkdirP(f"/data/kvrocks/{dbname}/data/backup");

	cfgstr = dbconfigbase;
	if masterIp is not None:
		dbconfigslave = f'''slaveof {masterIp} {port}
masterauth {password}
'''
		cfgstr += dbconfigslave;
	PutConfigFile(f"/data/kvrocks/{dbname}/conf/kvrocks.conf",cfgstr);

def CreateCavedb(name,ip,port,password,masterIp = None):
	dbname = name;

	if not dbname.startswith("cavedb-"):
		dbname = "cavedb-" + dbname;
	if not dbname.endswith("-%d"%port):
		dbname = dbname + "-%d"%port;

	cfg = {};
	cfg["logfile"] = f"/data/cavedb/{dbname}/log/cavedb.?.log"
	cfg["store"] = {"cache":10000,"path":f"/data/cavedb/{dbname}/db"}
	cfg["binlog"] = {"cache":5000,"path":f"/data/cavedb/{dbname}/binlog"}

	srvNetWork = {
		"type":"network",
		"host":ip,
		"port":port,
		"readonly":False if masterIp is None else True,
		"masterid":dbname,
		"requirepass":password,
	}

	srvUSOCK = {
		"type":"unixsocket",
		"unix":f"/data/cavedb/{dbname}/cavedb.sock",
		"readonly":False,
		"masterid":f"us-{dbname}",
	};
	cfg["server"] = [srvNetWork,srvUSOCK];

	if masterIp is not None:
		srvMaster = {
			"type":"cavedb",
			"host":masterIp,
			"port":port,
			"masterid":dbname,
			"masterauth":password,
		}
		cfg["slaveof"] = [srvMaster];

	cfgstr = json.dumps(cfg,indent=4);

	MkdirP(f"/data/cavedb/{dbname}");
	PutConfigFile(f"/data/cavedb/{dbname}/cavedb.conf",cfgstr);


def CreateRedis(name,ip,port,password,masterIp = None):
	dbname = name;

	if not dbname.startswith("redis-"):
		dbname = "redis-" + dbname;
	if not dbname.endswith("-%d"%port):
		dbname = dbname + "-%d"%port;
	cfg1 = f'''
bind 127.0.0.1 -::1 {ip}
port {port}
dir "/data/redis/{dbname}/data"
pidfile "/data/redis/{dbname}/redis.pid"
unixsocket "/data/redis/{dbname}/redis.sock"
logfile "/data/redis/{dbname}/logs/redis.log"
requirepass "{password}"
'''
	cfg2 = f'''slaveof {masterIp} {port}
masterauth "{password}"
'''
	cfg3 = '''protected-mode yes
tcp-backlog 511
timeout 0
tcp-keepalive 300
daemonize yes
loglevel notice
syslog-enabled yes
databases 16
always-show-logo no
set-proc-title yes
proc-title-template "{title} {listen-addr} {server-mode}"
save 900 1
save 300 10
save 60 10000
stop-writes-on-bgsave-error yes
rdbcompression yes
rdbchecksum yes
dbfilename "develop-6379.rdb"
rdb-del-sync-files no
replica-serve-stale-data yes
replica-read-only yes
repl-diskless-sync yes
repl-diskless-sync-delay 5
repl-diskless-sync-max-replicas 0
repl-diskless-load disabled
repl-disable-tcp-nodelay no
replica-priority 100
acllog-max-len 128
maxclients 20000
maxmemory 4gb
lazyfree-lazy-eviction no
lazyfree-lazy-expire no
lazyfree-lazy-server-del no
replica-lazy-flush no
lazyfree-lazy-user-del no
lazyfree-lazy-user-flush no
oom-score-adj no
oom-score-adj-values 0 200 800
disable-thp yes
appendonly no
appendfilename "develop-6379.aof"
appenddirname "appendonlydir"
appendfsync everysec
no-appendfsync-on-rewrite no
auto-aof-rewrite-percentage 100
auto-aof-rewrite-min-size 64mb
aof-load-truncated yes
aof-use-rdb-preamble yes
aof-timestamp-enabled no
slowlog-log-slower-than 10000
slowlog-max-len 128
latency-monitor-threshold 0
notify-keyspace-events ""
hash-max-listpack-entries 512
hash-max-listpack-value 64
list-max-listpack-size -2
list-compress-depth 0
set-max-intset-entries 512
zset-max-listpack-entries 128
zset-max-listpack-value 64
hll-sparse-max-bytes 3000
stream-node-max-bytes 4kb
stream-node-max-entries 100
activerehashing yes
client-output-buffer-limit normal 0 0 0
client-output-buffer-limit replica 256mb 64mb 60
client-output-buffer-limit pubsub 32mb 8mb 60
hz 10
dynamic-hz yes
aof-rewrite-incremental-fsync yes
rdb-save-incremental-fsync yes
jemalloc-bg-thread yes
'''
	MkdirP(f"/data/redis/{dbname}/data");
	MkdirP(f"/data/redis/{dbname}/logs");

	cfgstr = cfg1;
	if masterIp is not None:
		cfgstr += cfg2;
	cfgstr += cfg3;
	PutConfigFile(f"/data/redis/{dbname}/redis.conf",cfgstr);




# name,ip,port,password,masterIp

def useage():
	print("需要至少5个参数 type name ip port password [masterIp]");
	print("type可选的值有redis、kvrocks、cavedb");
	print("举个例子：");
	print("python3 CreateDataBase.py cavedb mydb 192.168.100.121 7951 mypassword 192.168.100.122");

if __name__ == "__main__":
	import sys;
	if len(sys.argv) >= 6:
		dbtype = sys.argv[1];
		if dbtype not in ("cavedb","kvrocks","redis"):
			useage();
			exit(0);
		name = sys.argv[2];
		ip = sys.argv[3];
		port = int(sys.argv[4]);
		password = sys.argv[5];
		masterIp = None;
		if len(sys.argv) > 6:
			masterIp = sys.argv[6];

		if dbtype == "cavedb":
			CreateCavedb(name,ip,port,password,masterIp);

		if dbtype == "kvrocks":
			CreateKvrocks(name,ip,port,password,masterIp);

		if dbtype == "redis":
			CreateRedis(name,ip,port,password,masterIp);
	else:
		useage();