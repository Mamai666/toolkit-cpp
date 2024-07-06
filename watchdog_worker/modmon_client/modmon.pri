INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/ModMonClient.cpp

HEADERS += \
    $$PWD/ModMonClient.h

INCLUDEPATH *= $$PWD/../../message_transport/include
LIBS *= -L$$PWD/toolkit-cpp/message_transport/lib/$$QT_ARCH -lTransportClient -ldl

QMAKE_RPATHDIR +=/opt/$${COMPANY}/$${SUFX}-libs/message_transport
