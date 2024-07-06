COMPANY = lms
SUFX    = mtp

INCLUDEPATH += $$PWD/include

message($$QT_ARCH)

LIBS += -L$$PWD/lib/$$QT_ARCH -lamqpcpp
LIBS += -lpthread -ldl -lev
QMAKE_RPATHDIR +=/opt/$${COMPANY}/$${SUFX}-libs/rabbitmq
