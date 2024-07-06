# Проверка версии CMake
cmake_minimum_required( VERSION 3.11 )
# Если версия установленой программы ниже, то ошибка выполнения

# Название проекта и проверка доступности компиляторя с++
project( pipeclient LANGUAGES CXX )			

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
message( STATUS "PROJECT: ${PROJECT_NAME}; Architecture: ${ARCHITECTURE}" )

# Установка переменной со списком исходников
include_directories(
		./include 
		../
		../../ 
		../../include
		../../../ 
		../../../loggerpp/include 
	)
	
set( SOURCES_LIB 
		${CMAKE_CURRENT_SOURCE_DIR}/../../../Utils/mstrings.cpp
    	${CMAKE_CURRENT_SOURCE_DIR}/../../src/BaseTransportAny.cpp 
		${CMAKE_CURRENT_SOURCE_DIR}/../../src/BaseTransportClient.cpp 
		${CMAKE_CURRENT_SOURCE_DIR}/../PipeTransportKit.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/PipeClient.cpp
	)
set( HEADERS_LIB 
		${CMAKE_CURRENT_SOURCE_DIR}/../../../Utils/mstrings.h
    	${CMAKE_CURRENT_SOURCE_DIR}/../../include/BaseTransportAny.h 
		${CMAKE_CURRENT_SOURCE_DIR}/../../include/BaseTransportClient.h
		${CMAKE_CURRENT_SOURCE_DIR}/../PipeTransportKit.h
		${CMAKE_CURRENT_SOURCE_DIR}/include/PipeClient.h
	)		

set(RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/${ARCHITECTURE}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/${ARCHITECTURE}")

# Включение экспорта всех символов для создания динамической библиотеки
#set ( CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON )

# Создание динамической библиотеки с именем PipeClient
add_library( pipeclient SHARED ${HEADERS_LIB} ${SOURCES_LIB} )	
install(TARGETS pipeclient DESTINATION /opt/lms/mtp-libs/message_transport)
