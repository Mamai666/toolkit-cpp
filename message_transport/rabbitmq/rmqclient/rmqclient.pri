COMPANY = lms
SUFX    = mtp

CONFIG += c++14

include($$PWD/../3rdparty/AMQP-CPP/amqpcpp.pri)

INCLUDEPATH += $$PWD \
            $$PWD/include \
            $$PWD/../ \
            $$PWD/../../ \
            $$PWD/../../../

HEADERS *= \
        $$PWD/../OwnLibEvHandler.h \
        $$PWD/include/RabbitMQClient.h
        
SOURCES *= \
        $$PWD/../OwnLibEvHandler.cpp \
        $$PWD/src/RabbitMQClient.cpp
        
#LIBS += -L$$PWD/lib/$$QT_ARCH -lconfigmgr
#QMAKE_RPATHDIR +=/opt/$${COMPANY}/$${SUFX}-libs/configmgr
