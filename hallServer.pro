QT -= gui

QT += network sql

CONFIG += c++11 console

SOURCES += \
    main.cpp \
    mytcpserver.cpp \

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    mytcpserver.h \

TARGET = gameServer