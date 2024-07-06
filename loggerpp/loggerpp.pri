COMPANY = lms
SUFX    = mtp

INCLUDEPATH += $$PWD \
            $$PWD/include
				
HEADERS += \
        $$PWD/include/LoggerPP.h

# SOURCES += \
#         $$PWD/src/LoggerPP.cpp

include(../Utils/utils.pri)
include(../DirManager/dirman.pri)

LIBS += -lpthread
LIBS += -L$$PWD/lib/$$QT_ARCH -lloggerpp

QMAKE_RPATHDIR +=/opt/$${COMPANY}/$${SUFX}-libs/loggerpp
