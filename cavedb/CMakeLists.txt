# cmake -D release=1 -D CMAKE_INSTALL_PREFIX=~/.local ..

#定义工程名及版本
project(cavedb)
set(version 3.0.0)
set(soversion 3.0)


cmake_minimum_required (VERSION 3.0.0) 
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)
set(CMAKE_USE_RELATIVE_PATHS ON)
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
set(CMAKE_BUILE_TYPE DEBUG) 


#是否开启内存泄露测试
set(MEMLEAKTEST 0)

#导入一此需要用到的工具包
include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckIncludeFileCXX)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckCSourceCompiles)
include(FindPkgConfig)
include(UsePkgConfig)
enable_testing()

#定制编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -Wall -fPIC -Bsymbolic")

if(release)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG ")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -std=c99 -O2 -DNDEBUG ")
	set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -O2")
	message("Build as release")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_DEBUG -g -fstack-protector ")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -D_DEBUG -std=c99 -g -fstack-protector ")
	set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -g -fstack-protector ")
	message("Build as debug")
endif()

#定义安装目录
set(CMAKE_INSTALL_PREFIX $ENV{HOME}/.local)

if(NOT share_type)
	set(share_type SHARED)
endif()

#搜索源代码
aux_source_directory(src src_cpps)

#搜索头文件
file(GLOB src_hs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.h)
include_directories(src)

aux_source_directory(src/redis src_cpps)
include_directories(src/redis)

aux_source_directory(src/ssdb src_cpps)
include_directories(src/ssdb)

aux_source_directory(src/store src_cpps)
include_directories(src/store)
file(GLOB src_hs_store RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/store/*.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

list(APPEND LIBS_DEPENDS -ldl)

## 搜索可选的外部依赖

if(NOT minimal)
	pkg_check_modules(LIBMILK libmilk REQUIRED)
	pkg_check_modules(LEVELDB leveldb>=1.19)
	pkg_check_modules(ZSTD libzstd)
	pkg_check_modules(LZ4 liblz4)


	#pkg_check_modules(SPARSEHASH sparsehash)
	#pkg_check_modules(LMDB lmdb)

	#check_include_file_cxx(snappy.h Z_HAVE_SNAPPY)
	#check_include_file_cxx(rocksdb/db.h Z_HAVE_ROCKSDB)
	##check_include_file_cxx(/usr/include/google/sparsehash/sparsehashtable.h Z_HAVE_SPARSEHASH)
	check_include_file_cxx("/etc/alternatives/java_sdk/include/jni.h" Z_HAVE_JDK)
endif()

if(LIBMILK_FOUND)
	list(APPEND BUILD_DEPENDS libmilk-devel)
	list(APPEND RUN_DEPENDS libmilk)
	list(APPEND LIBS_DEPENDS ${LIBMILK_LDFLAGS})
	list(APPEND INCS_DEPENDS ${LIBMILK_CFLAGS})
	list(APPEND CONFIG_MACROS LIBMILK_FOUND)
endif()


if(NOT minimal)
	if(LEVELDB_FOUND)
		#list(APPEND BUILD_DEPENDS leveldb-devel)
		#list(APPEND RUN_DEPENDS leveldb)
		list(APPEND LIBS_DEPENDS ${LEVELDB_LDFLAGS})
		list(APPEND INCS_DEPENDS ${LEVELDB_CFLAGS})
		list(APPEND CONFIG_MACROS LEVELDB_FOUND)
	endif()

	if(ZSTD_FOUND)
		list(APPEND LIBS_DEPENDS ${ZSTD_LDFLAGS})
		list(APPEND INCS_DEPENDS ${ZSTD_CFLAGS})
		list(APPEND CONFIG_MACROS ZSTD_FOUND)
	endif()

	if(LZ4_FOUND)
		list(APPEND LIBS_DEPENDS ${LZ4_LDFLAGS})
		list(APPEND INCS_DEPENDS ${LZ4_CFLAGS})
		list(APPEND CONFIG_MACROS LZ4_FOUND)
	endif()
endif()





if(Z_HAVE_JDK)
	list(APPEND INCS_DEPENDS -I"/etc/alternatives/java_sdk/include/"  -I"/etc/alternatives/java_sdk/include/linux/")
	list(APPEND INCS_DEPENDS -I"/etc/alternatives/java_sdk/include/"  -I"/etc/alternatives/java_sdk/include/linux/")
else()
	list(REMOVE_ITEM src_cpps src/jni.cpp)
endif()

if(UNIX)
	set(LIBRARIES -lpthread)
else()
	message(FATAL_ERROR "cavedb is works on Linux only!!!")
endif()

#根据外部依赖的存在与定义一些宏
foreach(v ${BUILD_DEPENDS})
	set(CMAKE_BUILD_DEPENDS "${CMAKE_BUILD_DEPENDS} ${v}")
endforeach()

foreach(v ${RUN_DEPENDS})
	set(CMAKE_RUN_DEPENDS "${CMAKE_RUN_DEPENDS} ${v}")
endforeach()

foreach(v ${INCS_DEPENDS})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${v}")
endforeach()

set(CMAKE_LIBS_DEPENDS ${LIBS_DEPENDS})

foreach(v ${CONFIG_MACROS})
	set(CMAKE_CONFIG_MACROS "${CMAKE_CONFIG_MACROS}#define ${v}\n")
endforeach()

#修改配置文件
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)


#目标动态库
add_library(${PROJECT_NAME} ${share_type} ${src_cpps} ${src_hs} ${src_hs_store})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${version} SOVERSION ${soversion})

#拷贝文件
file(COPY ${src_hs} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)
file(COPY ${src_hs_store} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/store)


if(NOT minimal)
	if(NOT LEVELDB_FOUND)
		#对leveldb的依赖
		#execute_process(COMMAND  chmod u+x  ${CMAKE_CURRENT_SOURCE_DIR}/depends/leveldb/build_detect_platform)
		execute_process(COMMAND  make -j -C ${CMAKE_CURRENT_SOURCE_DIR}/depends/leveldb/ out-static/libleveldb.a out-static/libmemenv.a)
		include_directories(${CMAKE_CURRENT_SOURCE_DIR}/depends/leveldb/include/)
		target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/depends/leveldb/out-static/libleveldb.a)
		target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/depends/leveldb/out-static/libmemenv.a)
	endif()
endif()




#对libmilk的依赖
if(NOT ${LIBMILK_FOUND})
	add_custom_target(milk COMMAND make -C /usr/local/src/libmilk/build)
	add_dependencies(${PROJECT_NAME} milk)
endif()

#链接库
if(UNIX)
	 target_link_libraries(${PROJECT_NAME} -lpthread ${CMAKE_LIBS_DEPENDS})
else()
	message(FATAL_ERROR "cavedb is works on Linux only!!!")
endif()

#定义应用目标
file(GLOB src_app RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/app/*.cc)
foreach(var ${src_app})
	string(REGEX REPLACE "src/app/(.*).cc" "\\1" var2 ${var})
	add_executable(${var2} ${var})
	target_link_libraries(${var2} ${PROJECT_NAME})
	#target_link_libraries(${var2} -L/usr/lib64/mysql -lmysqlclient)
	#install(TARGETS ${PROJECT_NAME} DESTINATION ${install_dir})
endforeach()

#定义安装目标
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pkg/${PROJECT_NAME}.pc DESTINATION lib64/pkgconfig)
install(FILES ${src_hs} DESTINATION include/cavedb)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/config.h DESTINATION include/cavedb)
install(FILES ${src_hs_store} DESTINATION include/cavedb/store/)

install(TARGETS ${PROJECT_NAME} DESTINATION lib64)
install(TARGETS caved DESTINATION bin)

#单元测试，是否开启内存泄露测试
if(MEMLEAKTEST)
	set(unittest valgrind --tool=memcheck --leak-check=full)
else()
	set(unittest )
endif()

file(GLOB src_test RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} testsuite/*.cc)
foreach(var ${src_test})
	string(REGEX REPLACE "testsuite/(.*).cc" "\\1" var2 ${var})
	add_executable(${var2} ${var})
	target_link_libraries(${var2} ${PROJECT_NAME})
	add_test(NAME ${var2} COMMAND ${unittest} ./${var2})
endforeach()
