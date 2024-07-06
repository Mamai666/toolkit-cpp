# Проверка версии CMake
cmake_minimum_required( VERSION 3.11 )
# Если версия установленой программы ниже, то ошибка выполнения

# Название проекта и проверка доступности компиляторя с++
project( BaseTransports LANGUAGES CXX)			

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
message( STATUS "PROJECT: ${PROJECT_NAME}; Architecture: ${ARCHITECTURE}" )

# Установка переменной со списком исходников
include_directories(
        ../json
        ../
		../loggerpp/include
		./include
	)
set( SOURCES_LIB_CL 
        ${CMAKE_CURRENT_SOURCE_DIR}/../Utils/files.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/BaseTransportClient.cpp 
		${CMAKE_CURRENT_SOURCE_DIR}/src/BaseTransportAny.cpp
	)
set( HEADERS_LIB_CL
		${CMAKE_CURRENT_SOURCE_DIR}/include/BaseTransportClient.h 
		${CMAKE_CURRENT_SOURCE_DIR}/include/BaseTransportAny.h 
	)		

set( SOURCES_LIB_SR 
        ${CMAKE_CURRENT_SOURCE_DIR}/../Utils/files.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/BaseTransportServer.cpp 
		${CMAKE_CURRENT_SOURCE_DIR}/src/BaseTransportAny.cpp
	)
set( HEADERS_LIB_SR
		${CMAKE_CURRENT_SOURCE_DIR}/include/BaseTransportServer.h 
		${CMAKE_CURRENT_SOURCE_DIR}/include/BaseTransportAny.h 
	)		

set(RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/${ARCHITECTURE}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/${ARCHITECTURE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/${ARCHITECTURE}")

# Включение экспорта всех символов для создания динамической библиотеки
#set ( CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON )

# Создание динамической библиотеки
#set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
add_library( TransportServer STATIC ${SOURCES_LIB_SR} )
#add_library( TransportServer SHARED ${HEADERS_LIB_SR} ${SOURCES_LIB_SR} )
target_link_libraries(TransportServer dl)

add_library( TransportClient STATIC ${SOURCES_LIB_CL} )
#add_library( TransportClient SHARED ${HEADERS_LIB_CL} ${SOURCES_LIB_CL} )
target_link_libraries(TransportClient dl)

add_subdirectory(./dummy_client)
add_subdirectory(./dummy_server)

add_subdirectory(./websocket++/client)
add_subdirectory(./websocket++/server)

add_subdirectory(./rabbitmq/rmqclient)
add_subdirectory(./rabbitmq/rmqserver)

add_subdirectory(./pipe/pipeclient)
add_subdirectory(./pipe/pipeserver)
