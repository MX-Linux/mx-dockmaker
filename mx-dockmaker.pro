# **********************************************************************
# * Copyright (C) 2020 MX Authors
# *
# * Authors: Adrian
# *          MX Linux <http://mxlinux.org>
# *
# * This is free software: you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation, either version 3 of the License, or
# * (at your option) any later version.
# *
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License
# * along with this package. If not, see <http://www.gnu.org/licenses/>.
# **********************************************************************/

QT       += core gui
CONFIG   += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = mx-dockmaker
TEMPLATE = app


SOURCES += main.cpp\
    mainwindow.cpp \
    about.cpp \
    cmd.cpp \
    picklocation.cpp

HEADERS  += \
    mainwindow.h \
    version.h \
    about.h \
    cmd.h \
    picklocation.h

FORMS    += \
    mainwindow.ui \
    picklocation.ui

TRANSLATIONS += translations/mx-dockmaker_ca.ts \
                translations/mx-dockmaker_de.ts \
                translations/mx-dockmaker_el.ts \
                translations/mx-dockmaker_es.ts \
                translations/mx-dockmaker_fr.ts \
                translations/mx-dockmaker_it.ts \
                translations/mx-dockmaker_ja.ts \
                translations/mx-dockmaker_nl.ts \
                translations/mx-dockmaker_ro.ts \
                translations/mx-dockmaker_sv.ts

RESOURCES += \
    images.qrc

