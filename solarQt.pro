#-------------------------------------------------
#
# Project created by QtCreator 2014-03-15T23:31:39
#
#-------------------------------------------------

QT       += core network
QT       -= gui

TARGET = solarQt

OBJECTS_DIR =./obj
MOC_DIR = ./obj
DESTDIR = ./bin

CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += include

SOURCES += src/main.cpp \
    src/inverter.cpp

HEADERS += \
    include/inverter.h
