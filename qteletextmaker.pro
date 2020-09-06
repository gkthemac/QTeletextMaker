QT += widgets
requires(qtConfig(filedialog))

HEADERS       = document.h \
                mainwidget.h \
                mainwindow.h \
                page.h \
                pageoptionsdockwidget.h \
                palettedockwidget.h \
                render.h \
                x26dockwidget.h \
                x26model.h \
                x26triplets.h \
                x28dockwidget.h
SOURCES       = document.cpp \
                main.cpp \
                mainwidget.cpp \
                mainwindow.cpp \
                pageoptionsdockwidget.cpp \
                palettedockwidget.cpp \
                page.cpp \
                render.cpp \
                x26dockwidget.cpp \
                x26model.cpp \
                x26triplets.cpp \
                x28dockwidget.cpp
RESOURCES     = qteletextmaker.qrc

# install
target.path = /usr/local/bin
INSTALLS += target
