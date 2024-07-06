INCLUDEPATH += $$PWD \
            $$PWD/include \
            $$PWD/../../ \
            $$PWD/../../../

include($$PWD/../../../websocketpp/websocketpp.pri)

HEADERS *= \
        $$PWD/include/WebSocketClient.h
        
SOURCES *= \
        $$PWD/src/WebSocketClient.cpp
