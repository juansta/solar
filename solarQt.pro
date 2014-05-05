#-------------------------------------------------
#
# Project created by QtCreator 2014-03-15T23:31:39
#
#-------------------------------------------------

QT       += core network
QT       -= gui

TARGET = solarQt

OBJECTS_DIR =./obj
DESTDIR = ./bin

CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDE_PATH += ./include

SOURCES += src/main.cpp \
    src/inverter.cpp

HEADERS += \
    src/inverter.h
