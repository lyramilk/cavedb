cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0048 NEW)
#定义工程名及版本
project(cavedb VERSION 3.0.0)
set(soversion 3.0)

# 包含公共代码*
if(NOT SUBMODULE)
	set(SUBMODULE ${PROJECT_SOURCE_DIR}/3rd_party)
endif()
include(${SUBMODULE}/libmilk/libmilk/libmilk.cmake)

# 设置环境变量*
libmilk_cmake_init_env()

# 搜索代码
unset(codes)
unset(headers)

aux_source_directory(src codes)
file(GLOB headers RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/cavedb/*.h)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} include/cavedb)

#搜索子模块
set(CMAKE_CXX_STANDARD 11)

if(NOT TARGET milk)
	if(shared)
		add_subdirectory(${SUBMODULE}/libmilk/libmilk ${SUBMODULE_BIN}/libmilk)
	else()
		add_subdirectory(${SUBMODULE}/libmilk/libmilk ${SUBMODULE_BIN}/libmilk EXCLUDE_FROM_ALL)
	endif()
endif()

# leveldb中强制添加了 -fno-rtti 如果不加这个会导致编译失败找不到leveldb::Comparator
add_compile_options(-fno-rtti)

if(NOT TARGET leveldb)
	add_subdirectory(${SUBMODULE}/leveldb ${SUBMODULE_BIN}/leveldb EXCLUDE_FROM_ALL)
	include_directories(${SUBMODULE}/leveldb/include)
endif()


#搜索依赖
check_include_file_cxx("/etc/alternatives/java_sdk/include/jni.h" Z_HAVE_JDK)

if(Z_HAVE_JDK)
	list(APPEND INCS_DEPENDS -I"/etc/alternatives/java_sdk/include/"  -I"/etc/alternatives/java_sdk/include/linux/")
else()
	list(REMOVE_ITEM codes src/jni.cpp)
endif()


#定义目标*
libmilk_cmake_def(${PROJECT_NAME} ${PROJECT_NAME}-shared cavedb codes headers ${PROJECT_NAME}-alone)
if(${PROJECT_NAME}-shared)
	#target_link_libraries(${PROJECT_NAME} PUBLIC milk)
	target_link_libraries(${PROJECT_NAME} PRIVATE leveldb)
else()
	#target_link_libraries(${PROJECT_NAME} PUBLIC -Wl,--whole-archive milk_static -Wl,--no-whole-archive)
	target_link_libraries(${PROJECT_NAME} PRIVATE leveldb)
endif()

#定义主程序
add_executable(caved src/app/caved.cc)
install(TARGETS caved DESTINATION bin)
target_link_libraries(caved ${PROJECT_NAME} milk ${LIBS_DEPENDS})
