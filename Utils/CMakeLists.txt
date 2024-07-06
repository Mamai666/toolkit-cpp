# Проверка версии CMake
cmake_minimum_required( VERSION 3.11 )
# Если версия установленой программы ниже, то ошибка выполнения

# Название проекта и проверка доступности компиляторя с++
project( utils LANGUAGES CXX)			

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
message( STATUS "PROJECT: ${PROJECT_NAME}; Architecture: ${ARCHITECTURE} DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

# Установка переменной со списком исходников
include_directories(
		${CMAKE_CURRENT_SOURCE_DIR}/../
	)
set( SOURCES_LIB 
		${CMAKE_CURRENT_SOURCE_DIR}/maths.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/files.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/mstrings.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/elapsed_timer.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/FileMonitor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/uuid.cpp
	)
set( HEADERS_LIB
		${CMAKE_CURRENT_SOURCE_DIR}/maths.h
		${CMAKE_CURRENT_SOURCE_DIR}/files.h
		${CMAKE_CURRENT_SOURCE_DIR}/mstrings.h
		${CMAKE_CURRENT_SOURCE_DIR}/elapsed_timer.h
		${CMAKE_CURRENT_SOURCE_DIR}/FileMonitor.h
		${CMAKE_CURRENT_SOURCE_DIR}/uuid.h
	)

add_library(utils "")

target_sources(	utils 
  PRIVATE
    ${SOURCES_LIB}
  PUBLIC
    ${HEADERS_LIB}
    )
	
target_include_directories(utils
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
