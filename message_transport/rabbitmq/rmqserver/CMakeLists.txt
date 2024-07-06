# Проверка версии CMake
cmake_minimum_required( VERSION 3.11 )
# Если версия установленой программы ниже, то ошибка выполнения

# Название проекта и проверка доступности компиляторя с++
project( rmqserver LANGUAGES CXX)			

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
message( STATUS "PROJECT: ${PROJECT_NAME}; Architecture: ${ARCHITECTURE}" )

# Установка переменной со списком исходников
include_directories(
		./include
		../3rdparty/AMQP-CPP/include
		../ 
		../../include
		../../../
		../../../loggerpp/include
	)
set( SOURCES_LIB 
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../Utils/mstrings.cpp
    	${CMAKE_CURRENT_SOURCE_DIR}/../../src/BaseTransportAny.cpp 
		${CMAKE_CURRENT_SOURCE_DIR}/../../src/BaseTransportServer.cpp 
		${CMAKE_CURRENT_SOURCE_DIR}/../OwnLibEvHandler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/RabbitMQServer.cpp
	)
set( HEADERS_LIB
		${CMAKE_CURRENT_SOURCE_DIR}/../../../Utils/mstrings.h
    	${CMAKE_CURRENT_SOURCE_DIR}/../../include/BaseTransportAny.h
		${CMAKE_CURRENT_SOURCE_DIR}/../../include/BaseTransportServer.h 
		${CMAKE_CURRENT_SOURCE_DIR}/../OwnLibEvHandler.h
		${CMAKE_CURRENT_SOURCE_DIR}/include/RabbitMQServer.h
	)		

set(RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/${ARCHITECTURE}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/${ARCHITECTURE}")

# Включение экспорта всех символов для создания динамической библиотеки
#set ( CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON )

# Директория, где находится библиотека libamqpcpp
set(LIBAMQP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/AMQP-CPP/lib/${ARCHITECTURE}")

SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) 
SET(CMAKE_INSTALL_RPATH "/opt/lms/mtp-libs/rabbitmq")

# Создание динамической библиотеки
add_library( rmqserver SHARED ${HEADERS_LIB} ${SOURCES_LIB} )
target_link_libraries(rmqserver pthread dl ev ${LIBAMQP_DIR}/libamqpcpp.so)
install(TARGETS rmqserver DESTINATION /opt/lms/mtp-libs/message_transport)
