#-------------------------------------------------
#
# Project created by QtCreator 2011-07-04T09:09:04
#
#-------------------------------------------------

QT       += core
QT       -= gui

TARGET = pipgit
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

windows:{
   target.path=/usr/local/bin
} else {
   DEFINES+=PIPGIT_LINUX
   target.path=/usr/bin
}

CONFIG(debug, debug|release){
   DEFINES+=PIPGIT_DEBUG
}

INSTALLS=target

SOURCES += main.cpp
