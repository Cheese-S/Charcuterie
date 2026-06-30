cmake_minimum_required(VERSION 4.0)

project(
    milk_core
    LANGUAGES CXX
)

add_library(milk_core

    # BEGIN_FILE_GEN src/filesystem

	src/filesystem/File.cpp
	src/filesystem/Path.cpp
	src/filesystem/WindowsFileHandle.cpp

    # END_GEN

    # BEGIN_FILE_GEN src/memory

	src/memory/Memory.cpp

    # END_GEN

    # BEGIN_FILE_GEN src/container

	src/container/StringView.cpp

    # END_GEN

    # BEGIN_FILE_GEN src/log

	src/log/AnsiColor.cpp
	src/log/Formatter.cpp
	src/log/Log.cpp
	src/log/Sink.cpp

    # END_GEN

    # BEGIN_FILE_GEN src/concurrency

	src/concurrency/ParkingLot.cpp
	src/concurrency/SharedMutex.cpp
	src/concurrency/WordLock.cpp

    # END_GEN

    # BEGIN_FILE_GEN src/concurrency/jobsystem

	src/concurrency/jobsystem/JobSystem.cpp
	src/concurrency/jobsystem/WaitGroup.cpp

    # END_GEN

    # BEGIN_FILE_GEN src/util

	src/util/Clock.cpp

    # END_GEN

    # BEGIN_FILE_GEN src/io

	src/io/BufferedWriter.cpp

    # END_GEN

    # BEGIN_FILE_GEN src/math

	src/math/Mlm.cpp

    # END_GEN
)

target_include_directories(milk_core PUBLIC
    interface
)

target_include_directories(milk_core PRIVATE
    include
)

target_compile_options(milk_core PUBLIC
    ${cc_compile_options}
)

target_link_options(milk_core PUBLIC 
    ${cc_link_options}
)

# libs
target_link_libraries(milk_core PUBLIC mimalloc)
target_link_libraries(milk_core PUBLIC fmt)
target_link_libraries(milk_core PUBLIC MCMPQueue)
target_link_libraries(milk_core PUBLIC glm)

# tests
# BEGIN_TEST_GEN ./test milk_core


add_test(MemoryTest 
         SOURCES test/MemoryTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)

add_test(ConditionTest 
         SOURCES test/concurrency/ConditionTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)

add_test(JobQueueTest 
         SOURCES test/concurrency/JobQueueTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)

add_test(JobSystemTest 
         SOURCES test/concurrency/JobSystemTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)

add_test(ParkingLotTest 
         SOURCES test/concurrency/ParkingLotTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)

add_test(SharedMutexTest 
         SOURCES test/concurrency/SharedMutexTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)

add_test(WaitGroupTest 
         SOURCES test/concurrency/WaitGroupTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)

add_test(WordLockTest 
         SOURCES test/concurrency/WordLockTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)

add_test(FixedSizeObjectPoolTest 
         SOURCES test/container/FixedSizeObjectPoolTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)

add_test(LockFreeObjectPoolTest 
         SOURCES test/container/LockFreeObjectPoolTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)

add_test(StringTest 
         SOURCES test/container/StringTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)

add_test(VectorTest 
         SOURCES test/container/VectorTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)

add_test(FileTest 
         SOURCES test/filesystem/FileTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)

add_test(PathTest 
         SOURCES test/filesystem/PathTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)

add_test(BufferedWriterTest 
         SOURCES test/io/BufferedWriterTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)

add_test(LogTest 
         SOURCES test/log/LogTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)

add_test(MlmTest 
         SOURCES test/math/MlmTest.cpp 
         INCLUDES include interface milk
         LIBS milk_core milk_test)
# END_GEN

