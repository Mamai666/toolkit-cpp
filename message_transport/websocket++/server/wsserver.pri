INCLUDEPATH += $$PWD \
            $$PWD/include \
            $$PWD/../../ \
            $$PWD/../../../

include($$PWD/../../../websocketpp/websocketpp.pri)

HEADERS *= \
        $$PWD/include/WebSocketServer.h
        
SOURCES *= \
        $$PWD/src/WebSocketServer.cpp
