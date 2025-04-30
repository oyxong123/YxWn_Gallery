QT += core gui multimedia multimediawidgets
RC_ICONS = resources/Terriermon.ico

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    progressbarwindow.cpp \
    qcustompushbutton.cpp \
    settingswindow.cpp

HEADERS += \
    mainwindow.h \
    progressbarwindow.h \
    qcustompushbutton.h \
    settingswindow.h

FORMS += \
    mainwindow.ui \
    progressbarwindow.ui \
    settingswindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
