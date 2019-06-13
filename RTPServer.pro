TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    src/main.cpp \
    src/rtp/rtpsession.cpp \
    src/core/logger.cpp \
    src/serverengine.cpp \
    src/core/abstractthread.cpp \
    src/rtp/rtpusermanager.cpp \
    src/rtp/queue.cpp

LIBS += -L$$PWD/sdk/lib/ -ljrtp \
		-L$$PWD/sdk/lib/ -ljthread \
		-pthread

INCLUDEPATH += $$PWD/sdk/include
DEPENDPATH += $$PWD/sdk/include

HEADERS += \
    src/rtp/rtpsession.h \
    src/core/config.h \
    src/core/logger.h \
    src/core/error.h \
    src/serverengine.h \
    src/rtp/rtpusermanager.h \
    src/core/abstractqueue.h \
    src/core/abstractthread.h \
    src/rtp/queue.h

CONFIG(debug, debug|release): DEFINES += DEBUG
else:CONFIG(release, debug|release): DEFINES += NDEBUG
