.PYHONY: test header so

lyramilk/cave/cavedb.class:
	@echo "编译java"
	@javac *.java -d .


libcavedbjni.so:jni.cpp
	@echo "编译jni"
	g++ $^ -o $@ --shared -fPIC -lcavedb -I/usr/local/java/jdk1.8.0_151/include/ -I/usr/local/java/jdk1.8.0_151/include/linux

so:libcavedbjni.so
	@#

header: lyramilk/cave/cavedb.class
	javah -cp . lyramilk.cave.cavedb

test:lyramilk/cave/cavedb.class #libcavedbjni.so
	@echo "开始运行"
	@java lyramilk.cave.cavedb_test


