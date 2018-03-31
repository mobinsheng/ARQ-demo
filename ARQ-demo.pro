TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

LIBS += \
    -lpthread \

SOURCES += main.cpp \
    ikcp.c \
    arq_demo.cpp \
    time_func.cpp

HEADERS += \
    ikcp.h \
    arq_demo.h \
    udp_socket.h \
    time_func.h \
    client.h \
    server.h \
    task.h
