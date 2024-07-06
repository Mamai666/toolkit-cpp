# Проверка версии CMake
cmake_minimum_required( VERSION 3.11 )
# Если версия установленой программы ниже, то ошибка выполнения

# Название проекта и проверка доступности компиляторя с++
project( systemd_client LANGUAGES CXX)			

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
message( STATUS "PROJECT: ${PROJECT_NAME}; Architecture: ${ARCHITECTURE} DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

# Установка переменной со списком исходников
include_directories(
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../
	)
set( SOURCES_LIB 
		${CMAKE_CURRENT_SOURCE_DIR}/SystemdClient.cpp
	)
set( HEADERS_LIB
#		${CMAKE_CURRENT_SOURCE_DIR}/../../../Utils/mstrings.h
		${CMAKE_CURRENT_SOURCE_DIR}/SystemdClient.h
	)

add_library(systemd_client "")

target_sources(	systemd_client 
  PRIVATE
    ${SOURCES_LIB}
  PUBLIC
    ${HEADERS_LIB}
    )
	
target_include_directories(systemd_client
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
	
target_link_libraries(systemd_client systemd)
