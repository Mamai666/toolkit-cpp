# Проверка версии CMake
cmake_minimum_required( VERSION 3.11 )
# Если версия установленой программы ниже, то ошибка выполнения

# Название проекта и проверка доступности компиляторя с++
project( modmon_client LANGUAGES CXX)			

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
message( STATUS "PROJECT: ${PROJECT_NAME}; Architecture: ${ARCHITECTURE}" )

# Установка переменной со списком исходников
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../
		${CMAKE_CURRENT_SOURCE_DIR}/../../message_transport/include
	)
set( SOURCES_LIB 
        ${CMAKE_CURRENT_SOURCE_DIR}/../../message_transport/src/BaseTransportAny.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../message_transport/src/BaseTransportClient.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/ModMonClient.cpp
	)
set( HEADERS_LIB
#		${CMAKE_CURRENT_SOURCE_DIR}/../../../Utils/mstrings.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../message_transport/include/BaseTransportAny.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../message_transport/include/BaseTransportClient.h
		${CMAKE_CURRENT_SOURCE_DIR}/ModMonClient.h
	)
	
add_library(modmon_client "")

target_sources(	modmon_client 
  PRIVATE
    ${SOURCES_LIB}
  PUBLIC
    ${HEADERS_LIB}
    )
	
target_include_directories(modmon_client
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
