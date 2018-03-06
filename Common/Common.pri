DEFINES += SPDLOG_NO_THREAD_ID
DEFINES += SPDLOG_ENABLE_SYSLOG

INCLUDEPATH += $$PWD $$PWD/spdlog/include

SOURCES +=

HEADERS += \
        $$PWD/common.h \
        $$PWD/spdlog/include/spdlog/*.h
