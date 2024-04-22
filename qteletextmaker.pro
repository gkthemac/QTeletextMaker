QT += widgets
requires(qtConfig(filedialog))

HEADERS       = decode.h \
                document.h \
                hamming.h \
                keymap.h \
                levelonecommands.h \
                levelonepage.h \
                loadsave.h \
                mainwidget.h \
                mainwindow.h \
                pagebase.h \
                pagex26base.h \
                pagecomposelinksdockwidget.h \
                pageenhancementsdockwidget.h \
                pageoptionsdockwidget.h \
                palettedockwidget.h \
                render.h \
                x26commands.h \
                x26dockwidget.h \
                x26menus.h \
                x26model.h \
                x26triplets.h
SOURCES       = decode.cpp \
                document.cpp \
                levelonecommands.cpp \
                levelonepage.cpp \
                loadsave.cpp \
                main.cpp \
                mainwidget.cpp \
                mainwindow.cpp \
                pagebase.cpp \
                pagex26base.cpp \
                pagecomposelinksdockwidget.cpp \
                pageenhancementsdockwidget.cpp \
                pageoptionsdockwidget.cpp \
                palettedockwidget.cpp \
                render.cpp \
                x26commands.cpp \
                x26dockwidget.cpp \
                x26menus.cpp \
                x26model.cpp \
                x26triplets.cpp
RESOURCES     = qteletextmaker.qrc

# install
target.path = /usr/local/bin
INSTALLS += target
