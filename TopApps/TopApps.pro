#-------------------------------------------------
#
# Project created by QtCreator
#
#-------------------------------------------------

QT += core gui widgets

TARGET = TopApps

TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

QMAKE_CXXFLAGS += -Wpedantic
QMAKE_CXXFLAGS += -Wzero-as-null-pointer-constant

SOURCES += main.cpp topapps.cpp

HEADERS += topapps.h

RESOURCES += img.qrc

win32 {
LIBS += -lntdll

SOURCES += os_win.cpp

HEADERS += os_win.h

RC_FILE = res.rc
}
