QT       += core gui serialport widgets svg multimedia sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += qwt analogwidgets colorwidgets qmqtt #bibliotecas adicionales.

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    user_interface.cpp

HEADERS += \
    remotelink_messages.h \
    user_interface.h

FORMS += \
    user_interface.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
