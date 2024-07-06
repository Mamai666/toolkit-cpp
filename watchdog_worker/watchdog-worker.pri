INCLUDEPATH += $$PWD

include($$PWD/systemd_client/systemdcli.pri)
include($$PWD/modmon_client/modmon.pri)

SOURCES += \
    $$PWD/WatchDogWorker.cpp

HEADERS += \
    $$PWD/IWatchDogClient.h \
    $$PWD/WatchDogWorker.h
    
    
LIBS *= -no-pie -lpthread
