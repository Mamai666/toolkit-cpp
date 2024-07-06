COMPANY = lms
SUFX    = mtp

INCLUDEPATH += $$PWD \
            $$PWD/include
				
HEADERS *= \
        $$PWD/include/ConfigManager.h \
        $$PWD/include/ConfigMonitor.h \
        $$PWD/include/ConfigValidator.h \
        $$PWD/../Utils/FileMonitor.h
        
#SOURCES *= \
#        $$PWD/src/ConfigManager.cpp \
#        $$PWD/src/ConfigMonitor.cpp \
#        $$PWD/src/ConfigValidator.cpp
#        $$PWD/../Utils/FileMonitor.cpp

LIBS += -L$$PWD/lib/$$QT_ARCH -lconfigmgr
QMAKE_RPATHDIR +=/opt/$${COMPANY}/$${SUFX}-libs/configmgr
