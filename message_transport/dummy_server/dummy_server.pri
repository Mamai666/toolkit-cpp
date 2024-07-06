COMPANY = lms
SUFX    = mtp

INCLUDEPATH += $$PWD \
            $$PWD/include \
            $$PWD/../
				
HEADERS += \
        $$PWD/../include/BaseTransportAny.h \
        $$PWD/../include/BaseTransportServer.h \
        $$PWD/include/DummyServer.h

HEADERS += \
        $$PWD/src/DummyServer.cpp \
        $$PWD/../src/BaseTransportAny.cpp \
        $$PWD/../src/BaseTransportServer.cpp

#SOURCES += $$PWD/src/DummyServer.cpp

#LIBS += -L$$PWD/lib/$$QT_ARCH -lDummyServer
#QMAKE_RPATHDIR +=/opt/$${COMPANY}/$${SUFX}-libs/message_transport
