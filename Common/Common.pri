# Set options from spdlog's tweakme.h here so we can keep spdlog as a submodule.
DEFINES += SPDLOG_NO_THREAD_ID
DEFINES += SPDLOG_ENABLE_SYSLOG

INCLUDEPATH += $$PWD $$PWD/spdlog/include

SOURCES +=

HEADERS += \
        $$PWD/common.h \
        $$PWD/spdlog/include/spdlog/*.h
