cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0048 NEW)
#定义工程名及版本
project(cavedb VERSION 3.0.0)
set(soversion 3.0)

# 设置环境变量*
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)
set(CMAKE_USE_RELATIVE_PATHS ON)

option(shared "build dynamic library" OFF)
option(static "build static library" ON)
option(minimal "without third party" OFF)
option(alone "build without other dynamic library" OFF)

if(NOT DEFINED ${PROJECT_NAME}-shared)
	set(${PROJECT_NAME}-shared ${shared})
endif()
if(NOT DEFINED ${PROJECT_NAME}-static)
	set(${PROJECT_NAME}-static ${static})
endif()
if(NOT DEFINED ${PROJECT_NAME}-minimal)
	set(${PROJECT_NAME}-minimal ${minimal})
endif()
if(NOT DEFINED ${PROJECT_NAME}-alone)
	set(${PROJECT_NAME}-alone ${alone})
endif()

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_INSTALL_PREFIX $ENV{HOME}/.local)
set(CMAKE_CXX_STANDARD 11)

message("optional:CMAKE_BUILD_TYPE=" ${CMAKE_BUILD_TYPE})
message("optional:${PROJECT_NAME}-shared=" ${${PROJECT_NAME}-shared})
message("optional:${PROJECT_NAME}-static=" ${${PROJECT_NAME}-static})
message("optional:${PROJECT_NAME}-minimal=" ${${PROJECT_NAME}-minimal})
message("optional:${PROJECT_NAME}-alone=" ${${PROJECT_NAME}-alone})

# 搜索代码*
unset(codes)
unset(headers)

aux_source_directory(src codes)
file(GLOB headers RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/cavedb/*.h)
include_directories(include/cavedb)

# 包含公共代码*
include(3rd_party/libmilk/libmilk/libmilk.cmake)

#搜索子模块
set(milk-shared OFF)
set(milk-static ON)
set(milk-minimal ON)
add_subdirectory(3rd_party/libmilk/libmilk EXCLUDE_FROM_ALL)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/3rd_party/libmilk/libmilk/include)

# leveldb中强制添加了 -fno-rtti 如果不加这个会导致编译失败找不到leveldb::Comparator
add_compile_options(-fno-rtti)
add_compile_options(-fPIC)

add_subdirectory(3rd_party/leveldb EXCLUDE_FROM_ALL)
include_directories(3rd_party/leveldb/include)


#搜索依赖
check_include_file_cxx("/etc/alternatives/java_sdk/include/jni.h" Z_HAVE_JDK)

if(Z_HAVE_JDK)
	list(APPEND INCS_DEPENDS -I"/etc/alternatives/java_sdk/include/"  -I"/etc/alternatives/java_sdk/include/linux/")
else()
	list(REMOVE_ITEM codes src/jni.cpp)
endif()


#定义目标*
if(${PROJECT_NAME}-shared)
	libmilk_cmake_def_shared(${PROJECT_NAME} cavedb codes headers ${PROJECT_NAME}-alone)
	target_link_libraries(${PROJECT_NAME} PRIVATE milk_static)
	target_link_libraries(${PROJECT_NAME} PRIVATE leveldb)
endif()

if(${PROJECT_NAME}-static)
	libmilk_cmake_def_static(${PROJECT_NAME}_static cavedb codes headers ${PROJECT_NAME}-alone)
	target_link_libraries(${PROJECT_NAME}_static PRIVATE milk_static)
	target_link_libraries(${PROJECT_NAME}_static PRIVATE leveldb)
endif()

#定义主程序
add_executable(caved src/app/caved.cc)

if(${PROJECT_NAME}-static)
	target_link_libraries(caved ${PROJECT_NAME}_static)
elseif(${PROJECT_NAME}-shared)
	target_link_libraries(caved ${PROJECT_NAME})
endif()
