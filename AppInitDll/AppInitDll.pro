TEMPLATE = lib

CONFIG -= qt
CONFIG(release, debug|release):DEFINES += NDEBUG

QMAKE_LFLAGS += -static
QMAKE_LFLAGS += -nostdlib
contains(QMAKE_HOST.arch, x86_64) {
TARGET = topapps64

QMAKE_LFLAGS += -eDllEntryPoint
} else {
TARGET = topapps32

QMAKE_LFLAGS += -e_DllEntryPoint

LIBS += -lgcc #64bit div
}
QMAKE_CXXFLAGS += -Wpedantic
QMAKE_CXXFLAGS += -Wzero-as-null-pointer-constant

LIBS += -lkernel32

SOURCES += main.cpp
