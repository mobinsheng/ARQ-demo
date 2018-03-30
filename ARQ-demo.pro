TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

LIBS += \
    -lpthread \

SOURCES += main.cpp \
    test.cpp \
    ikcp.c \
    arq_demo.cpp

HEADERS += \
    ikcp.h \
    test.h \
    sync_queue.h \
    arq_demo.h
