cmake_minimum_required(VERSION 3.7)
project(MajordomoPattern0_1 C)

set(CMAKE_C_STANDARD 99)
link_libraries("-lmdp")
link_libraries("-lczmq")
add_library(include STATIC broker.c clientSynchronous.c workerCoffee.c clientJSONsynchronous.c workerJSONthreads.c)
add_library(DynamicQueueFIFO SHARED workerJSONthreads.c)
add_library(Queue SHARED workerJSONthreads_2.c)
link_libraries("-lzmq")
link_libraries("-ljson-c")
#link_libraries("-lmsgpackc")
link_libraries("-lpthread")
add_executable(BROKER broker.c include/client_task_asynchronous.h)
add_executable(worker workerCoffee.c)
add_executable(clientAsynchronous clientAsynchronous.c)
add_executable(clientSynchronous clientSynchronous.c)
add_executable(clientJSON_ASYNC clientJSONasynchronous.c)
add_executable(workerJSON workerJSON.c)
add_executable(workerJSONthreads workerJSONthreads.c)
add_executable(workerTHREAD2 workerJSONthreads_2.c Queue/queue.c)
target_link_libraries(workerJSONthreads DynamicQueueFIFO)
add_executable(clientJSON_SYNC clientJSONsynchronous.c)

add_executable(clientMSGPACK clientMSGPACK.c)