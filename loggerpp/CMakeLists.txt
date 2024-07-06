# Проверка версии CMake
cmake_minimum_required( VERSION 3.11 )
# Если версия установленой программы ниже, то ошибка выполнения

# Название проекта и проверка доступности компиляторя с++
project( loggerpp LANGUAGES CXX )			

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
message( STATUS "Architecture: ${ARCHITECTURE}" )

# Установка переменной со списком исходников
include_directories(./include ./3rdparty ../)

set( SOURCES_LIB 
     ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/easylogging++.cc
     ${CMAKE_CURRENT_SOURCE_DIR}/../Utils/files.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/../Utils/mstrings.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/LoggerPP.cpp
     )
     
set( HEADERS_LIB 
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/easylogging++.h 
    ${CMAKE_CURRENT_SOURCE_DIR}/../Utils/files.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../Utils/mstrings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/LoggerPP.h
    )		

set(RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/${ARCHITECTURE}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/${ARCHITECTURE}")

# Включение экспорта всех символов для создания динамической библиотеки
#set ( CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON )

#link_directories(./3rdparty)

# Создание динамической библиотеки с именем loggerpp
add_library( loggerpp SHARED ${HEADERS_LIB} ${SOURCES_LIB} )	
#target_link_libraries(loggerpp PRIVATE utils)
install(TARGETS loggerpp DESTINATION /opt/lms/mtp-libs/loggerpp)
