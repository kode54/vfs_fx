#-------------------------------------------------
#
# Project created by QtCreator 2012-12-24T12:47:27
#
#-------------------------------------------------

QT       -= core gui

TARGET = vfs_fx
TEMPLATE = lib

DEFINES += VFS_FX_LIBRARY

QMAKE_CFLAGS += -std=c99

LIBS += -L$$OUT_PWD/File_Extractor/prj/File_Extractor/

LIBS += -lFile_Extractor -lz

DEPENDPATH += $$PWD/File_Extractor/prj/File_Extractor

PRE_TARGETDEPS += $$OUT_PWD/File_Extractor/prj/File_Extractor/libFile_Extractor.a

INCLUDEPATH += File_Extractor

SOURCES += \
    vfs_fx.c

HEADERS +=

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
