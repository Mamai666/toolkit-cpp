COMPANY = lms
SUFX    = mtp

INCLUDEPATH += $$PWD \
            $$PWD/include
				
HEADERS += \
        $$PWD/../BaseTransportAny.h \
        $$PWD/../BaseTransportClient.h \
        $$PWD/include/DummyClient.h
        
SOURCES += \
        $$PWD/../src/BaseTransportAny.cpp \
        $$PWD/../src/BaseTransportClient.cpp \
        $$PWD/src/DummyClient.cpp

#LIBS += -L$$PWD/lib/$$QT_ARCH -lTransportClient
#QMAKE_RPATHDIR +=/opt/$${COMPANY}/$${SUFX}-libs/message_transport
