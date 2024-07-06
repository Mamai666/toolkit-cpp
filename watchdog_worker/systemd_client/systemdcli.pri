INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/SystemdClient.cpp

HEADERS += \
    $$PWD/SystemdClient.h
    
    
LIBS += -lsystemd
