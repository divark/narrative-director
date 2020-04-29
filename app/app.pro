QT       += core gui multimedia

CONFIG += c++17

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = narrative-director
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        main.cpp \
        narrativedirector.cpp \
        utilities/paragraphretriever.cpp \
    preferences.cpp \
    utilities/recordedpartstracker.cpp

HEADERS += \
        narrativedirector.h \
        utilities/paragraphretriever.h \
    preferences.h \
    utilities/recordedpartstracker.h

FORMS += \
        narrativedirector.ui \
    preferences.ui

DESTDIR = $$PWD/../build

INCLUDEPATH += \
	$$PWD \
	utilities
