QT += widgets
requires(qtConfig(filedialog))

HEADERS       = document.h \
                mainwidget.h \
                mainwindow.h \
                page.h \
                pageenhancementsdockwidget.h \
                pageoptionsdockwidget.h \
                palettedockwidget.h \
                render.h \
                x26dockwidget.h \
                x26model.h \
                x26triplets.h
SOURCES       = document.cpp \
                main.cpp \
                mainwidget.cpp \
                mainwindow.cpp \
                pageenhancementsdockwidget.cpp \
                pageoptionsdockwidget.cpp \
                palettedockwidget.cpp \
                page.cpp \
                render.cpp \
                x26dockwidget.cpp \
                x26model.cpp \
                x26triplets.cpp
RESOURCES     = qteletextmaker.qrc

# install
target.path = /usr/local/bin
INSTALLS += target
