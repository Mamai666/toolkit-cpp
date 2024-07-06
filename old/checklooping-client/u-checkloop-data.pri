INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/u-checkloop-client.cpp

HEADERS += \
    $$PWD/u-checkloop-client.hpp \
    $$PWD/u-checkloop-data.hpp

!win32:{
    LIBS += -lrt
}
