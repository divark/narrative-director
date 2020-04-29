QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES +=  tst_paragraphretrievertests.cpp \
        ../app/utilities/paragraphretriever.cpp
HEADERS += ../app/utilities/paragraphretriever.h
INCLUDEPATH += \
    ../app \
    ../app/utilities
