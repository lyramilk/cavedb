#定义工程名及版本
project(cavedb)
set(version 1.0.11)
set(soversion 1.0)


cmake_minimum_required (VERSION 2.8.11) 
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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -g -Wall")

if(release)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O4 -DNDEBUG -Wall ")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -std=c99 -O4 -DNDEBUG -Wall ")
	set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -O4 -Wall ")
	message("Build as release")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_DEBUG -g -Wall -fstack-protector ")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -D_DEBUG -std=c99 -g -Wall -fstack-protector ")
	set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -g -Wall -fstack-protector ")
	message("Build as debug")
endif()

#定义安装目录
if(NOT install_dir)
	set(install_dir ${CMAKE_CURRENT_SOURCE_DIR}/output)
endif()

#搜索源代码
aux_source_directory(src src_cpps)

#搜索头文件
file(GLOB src_hs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.h)
include_directories(src)

if(UNIX)
	aux_source_directory(src/redis src_cpps)
	include_directories(src/redis)
	aux_source_directory(src/ssdb src_cpps)
	include_directories(src/ssdb)
endif()

list(APPEND LIBS_DEPENDS -ldl)

## 搜索可选的外部依赖
if(UNIX)
	pkg_check_modules(LIBMILK libmilk)
	pkg_check_modules(LEVELDB leveldb)
endif()

if(NOT LIBMILK_FOUND)
	message(FATAL_ERROR "require rpm package libmilk-devel")
endif()

if(NOT LEVELDB_FOUND)
	message(FATAL_ERROR "require rpm package leveldb-devel")
endif()

if(LIBMILK_FOUND)
	list(APPEND BUILD_DEPENDS libmilk-devel)
	list(APPEND RUN_DEPENDS libmilk)
	list(APPEND LIBS_DEPENDS ${LIBMILK_LDFLAGS})
	list(APPEND INCS_DEPENDS ${LIBMILK_CFLAGS})
	list(APPEND CONFIG_MACROS LIBMILK_FOUND)
endif()

if(LEVELDB_FOUND)
	list(APPEND BUILD_DEPENDS leveldb-devel)
	list(APPEND RUN_DEPENDS leveldb)
	list(APPEND LIBS_DEPENDS ${LEVELDB_LDFLAGS})
	list(APPEND INCS_DEPENDS ${LEVELDB_CFLAGS})
	list(APPEND CONFIG_MACROS LEVELDB_FOUND)
else()
	list(REMOVE_ITEM src_cpps src/sni/hash_map.cpp)
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

message(${CMAKE_LIBS_DEPENDS})

foreach(v ${CONFIG_MACROS})
	set(CMAKE_CONFIG_MACROS "${CMAKE_CONFIG_MACROS}#define ${v}\n")
endforeach()

#修改配置文件
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pkg/cavedb.spec.in ${CMAKE_CURRENT_SOURCE_DIR}/pkg/cavedb.spec)


#目标动态库
add_library(${PROJECT_NAME} SHARED ${src_cpps} ${src_hs})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${version} SOVERSION ${soversion})

#对libmilk的依赖
if(NOT ${LIBMILK_CFLAGS})
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
	install(TARGETS ${PROJECT_NAME} DESTINATION ${install_dir})
endforeach()

#定义安装目标
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pkg/${PROJECT_NAME}.pc DESTINATION ${install_dir})
install(FILES ${src_hs} DESTINATION ${install_dir})
install(TARGETS ${PROJECT_NAME} DESTINATION ${install_dir})
install(TARGETS caved DESTINATION ${install_dir})

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
