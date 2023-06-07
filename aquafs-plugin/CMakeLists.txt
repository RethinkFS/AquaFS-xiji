cmake_minimum_required(VERSION 3.4)

if (NOT DEFINED AQUAFS_STANDALONE)
    set(AQUAFS_STANDALONE 1)
endif ()

if (NOT DEFINED AQUAFS_EXPORT_PROMETHEUS)
    set(AQUAFS_EXPORT_PROMETHEUS OFF)
endif ()

set(AQUAFS_VERSION v0.0.1-alpha)

set(aquafs_SOURCES_local "fs/fs_aquafs.cc" "fs/zbd_aquafs.cc" "fs/io_aquafs.cc" "fs/zonefs_aquafs.cc"
        "fs/zbdlib_aquafs.cc"
        "fs/raid/zone_raid.cc" "fs/raid/zone_raid_auto.cc" "fs/raid/zone_raid0.cc" "fs/raid/zone_raid1.cc" "fs/raid/zone_raidc.cc"
        "fs/raid/zone_raid_allocator.cc"
        "fs/configuration.cc"
        "fs/aquafs_utils.cc")
set(aquafs_HEADERS_local "fs/fs_aquafs.h" "fs/zbd_aquafs.h" "fs/io_aquafs.h" "fs/version.h" "fs/metrics.h"
        "fs/snapshot.h" "fs/filesystem_utility.h" "fs/zonefs_aquafs.h" "fs/zbdlib_aquafs.h"
        "fs/raid/zone_raid.h" "fs/raid/zone_raid_auto.h" "fs/raid/zone_raid0.h" "fs/raid/zone_raid1.h" "fs/raid/zone_raidc.h"
        "fs/raid/zone_raid_allocator.h"
        "fs/configuration.h"
        "fs/aquafs_utils.h")
set(aquafs_LIBS_local "zbd")
set(aquafs_COMPILE_FLAGS_local "")
set(aquafs_INCLUDE_PATHS_local ${CMAKE_CURRENT_SOURCE_DIR})

if (AQUAFS_RAID_URING)
    set(liburing4cpp_INC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/liburing4cpp/include)
    list(APPEND aquafs_INCLUDE_PATHS_local ${liburing4cpp_INC_PATH})
    # include headers
    file(GLOB liburing4cpp_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/liburing4cpp/include/liburing/*.hpp)
    # append and remove prefix for file items
    foreach (file ${liburing4cpp_HEADERS})
        get_filename_component(file_name ${file} NAME)
        list(APPEND aquafs_HEADERS_local "liburing4cpp/include/liburing/${file_name}")
    endforeach ()
    set(aquafs_COMPILE_FLAGS_local "${aquafs_COMPILE_FLAGS_local} -DAQUAFS_RAID_URING=1 -Wno-non-virtual-dtor -Wno-shadow")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(aquafs_COMPILE_FLAGS_local "${aquafs_COMPILE_FLAGS_local} -fcoroutines")
    endif ()
    list(APPEND aquafs_LIBS_local "uring" "fmt" "Threads::Threads")
endif ()

if (USE_RTTI)
    set(aquafs_COMPILE_FLAGS_local "${aquafs_COMPILE_FLAGS_local} -DUSE_RTTI=1")
endif ()

if (AQUAFS_DETECT_READ_OFFLINE)
    set(aquafs_COMPILE_FLAGS_local "${aquafs_COMPILE_FLAGS_local} -DAQUAFS_DETECT_READ_OFFLINE=1")
endif ()

if (AQUAFS_EXPORT_METRICS)
    set(aquafs_COMPILE_FLAGS_local "${aquafs_COMPILE_FLAGS_local} -DAQUAFS_EXPORT_METRICS=1")
endif ()

if (AQUAFS_SIM_DELAY)
    set(aquafs_COMPILE_FLAGS_local "${aquafs_COMPILE_FLAGS_local} -DAQUAFS_SIM_DELAY=1")
endif ()

if (AQUAFS_STANDALONE)
    project(aquafs)

    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/fs/version.h "#pragma once\n#define AQUAFS_VERSION \"${AQUAFS_VERSION}\"\n")

    set(aquafs_SOURCES ${aquafs_SOURCES_local})
    set(aquafs_HEADERS ${aquafs_HEADERS_local})
    set(aquafs_LIBS ${aquafs_LIBS_local} "gflags" "rocksdb" "uring" "zstd" "lz4" "snappy" "bz2" "z" "numa")

    #  file(GLOB_RECURSE port_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/port/*.cc)
    #  file(GLOB_RECURSE util_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/util/util/*.cc)
    #  file(GLOB_RECURSE test_util_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test_util/*.cc)

    if (NOT AQUAFS_SKIP_LIBZBD_CHECK)
        find_package(PkgConfig REQUIRED)
        pkg_search_module(ZBD REQUIRED libzbd>=1.5.0)
        # pkg_search_module(rocksdb REQUIRED)
        find_package(gflags REQUIRED)
        add_definitions(-DGFLAGS=1)
        include_directories(${GFLAGS_INCLUDE_DIR})
        # find_package(NUMA REQUIRED)
        add_definitions(-DNUMA)
        # include_directories(${NUMA_INCLUDE_DIR})
    endif ()

    #  add_library(port STATIC ${port_SOURCES})
    #  add_library(util STATIC ${util_SOURCES})

    include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/util)
    add_library(aaquafs STATIC ${aquafs_SOURCES} ${aquafs_HEADERS})
    target_link_libraries(aaquafs ${aquafs_LIBS})

    add_definitions(-DWITH_TERARKDB=1 -DOS_LINUX=1 -DROCKSDB_PLATFORM_POSIX=1)
    add_executable(aquafs ${CMAKE_CURRENT_SOURCE_DIR}/util/aquafs.cc)
    target_link_libraries(aquafs aaquafs)

    add_executable(defconfig ${CMAKE_CURRENT_SOURCE_DIR}/util/defconfig.cc)
    target_link_libraries(defconfig aaquafs)

    add_executable(db_bench ${CMAKE_CURRENT_SOURCE_DIR}/util/db_bench.cc)
    target_link_libraries(db_bench aaquafs)

    # basic test
    enable_testing()
    add_test(NAME aquafs-mkfs COMMAND sudo $<TARGET_FILE:aquafs> mkfs --zbd=nullb0 --aux_path=/tmp/aux_path --force)
    add_test(NAME aquafs-list COMMAND sudo $<TARGET_FILE:aquafs> list --zbd=nullb0)

    add_test(NAME aquafs-mkfs-raid0 COMMAND sudo $<TARGET_FILE:aquafs> mkfs --raids=raid0:dev:nullb0,dev:nullb1 --aux_path=/tmp/aux_path --force)
    add_test(NAME aquafs-mkfs-raid1 COMMAND sudo $<TARGET_FILE:aquafs> mkfs --raids=raid1:dev:nullb0,dev:nullb1 --aux_path=/tmp/aux_path --force)

    add_test(NAME aquafs-db_bench COMMAND sudo $<TARGET_FILE:db_bench> --fs_uri=aquafs://dev:nullb0 --benchmarks=fillrandom --use_direct_io_for_flush_and_compaction)
else ()
    set(aquafs_LIBS "zbd" PARENT_SCOPE)
    set(aquafs_CMAKE_EXE_LINKER_FLAGS "-u aquafs_filesystems_reg" PARENT_SCOPE)
    set(aquafs_INCLUDE_PATHS "fs" PARENT_SCOPE)

    if (NOT AQUAFS_SKIP_LIBZBD_CHECK)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(ZBD REQUIRED libzbd>=1.5.0)
    endif ()

    if (AQUAFS_EXPORT_PROMETHEUS)
        message(STATUS "Prometheus metrics enabled")
        # pkg_check_modules(prometheus REQUIRED prometheus-cpp-pull=1.1.0)
        find_package(prometheus-cpp REQUIRED)
        # link_libraries(prometheus-cpp-pull)
        list(APPEND aquafs_LIBS_local "prometheus-cpp-pull" "prometheus-cpp-core")
        list(APPEND aquafs_SOURCES_local "fs/metrics_prometheus.cc")
        list(APPEND aquafs_HEADERS_local "fs/metrics_prometheus.h")
        set(aquafs_COMPILE_FLAGS_local "${aquafs_COMPILE_FLAGS_local} -DAQUAFS_EXPORT_PROMETHEUS=1")
        # add_definitions(-DAQUAFS_EXPORT_PROMETHEUS=1)
    endif ()
    set(aquafs_HEADERS ${aquafs_HEADERS_local} PARENT_SCOPE)
    set(aquafs_SOURCES ${aquafs_SOURCES_local} PARENT_SCOPE)
    set(aquafs_LIBS ${aquafs_LIBS_local} PARENT_SCOPE)
    set(aquafs_COMPILE_FLAGS ${aquafs_COMPILE_FLAGS_local} PARENT_SCOPE)
    set(aquafs_INCLUDE_PATHS ${aquafs_INCLUDE_PATHS_local} PARENT_SCOPE)

    #  execute_process(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    #    RESULT_VARIABLE GENVER_RESULT
    #    COMMAND ./generate-version.sh)
    #
    #  if(GENVER_RESULT AND NOT GENVER_RESULT EQUAL 0)
    #    message(FATAL_ERROR "Generating AquaFS version failed")
    #  endif()

    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/fs/version.h "#pragma once\n#define AQUAFS_VERSION \"${AQUAFS_VERSION}\"\n")

    add_executable(aquafs ${CMAKE_CURRENT_SOURCE_DIR}/util/aquafs.cc ${CMAKE_CURRENT_SOURCE_DIR}/util/tools/tools.cc)
    target_link_libraries(aquafs rocksdb)
    target_include_directories(aquafs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    add_executable(defconfig ${CMAKE_CURRENT_SOURCE_DIR}/util/defconfig.cc)
    target_link_libraries(defconfig rocksdb)
    target_include_directories(defconfig PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # add unit tests
    file(GLOB test_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/util/unit_tests/*.cc")
    foreach (test_SOURCE ${test_SOURCES})
        get_filename_component(test_NAME ${test_SOURCE} NAME_WE)
        add_executable(${test_NAME} ${test_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/util/tools/tools.cc)
        target_link_libraries(${test_NAME} rocksdb)
        target_include_directories(${test_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        # add_test(NAME ${test_NAME} COMMAND ${test_NAME})
    endforeach ()

    add_test(NAME aquafs-mkfs COMMAND sudo $<TARGET_FILE:aquafs> mkfs --zbd=nullb0 --aux_path=/tmp/aux_path --force)
    add_test(NAME aquafs-list COMMAND sudo $<TARGET_FILE:aquafs> list --zbd=nullb0)

    add_test(NAME aquafs-mkfs-raid0 COMMAND sudo $<TARGET_FILE:aquafs> mkfs --raids=raid0:dev:nullb0,dev:nullb1 --aux_path=/tmp/aux_path --force)
    add_test(NAME aquafs-mkfs-raid1 COMMAND sudo $<TARGET_FILE:aquafs> mkfs --raids=raid1:dev:nullb0,dev:nullb1 --aux_path=/tmp/aux_path --force)

    add_test(NAME aquafs-db_bench COMMAND sudo $<TARGET_FILE:db_bench> --fs_uri=aquafs://raid1:dev:nullb0,dev:nullb1 --benchmarks=fillrandom --use_direct_io_for_flush_and_compaction)
endif ()