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

TRANSLATIONS += translations/mx-dockmaker_am.ts \
                translations/mx-dockmaker_ar.ts \
                translations/mx-dockmaker_bg.ts \
                translations/mx-dockmaker_bn.ts \
                translations/mx-dockmaker_ca.ts \
                translations/mx-dockmaker_cs.ts \
                translations/mx-dockmaker_da.ts \
                translations/mx-dockmaker_de.ts \
                translations/mx-dockmaker_el.ts \
                translations/mx-dockmaker_es.ts \
                translations/mx-dockmaker_et.ts \
                translations/mx-dockmaker_eu.ts \
                translations/mx-dockmaker_fa.ts \
                translations/mx-dockmaker_fil_PH.ts \
                translations/mx-dockmaker_fi.ts \
                translations/mx-dockmaker_fr.ts \
                translations/mx-dockmaker_he_IL.ts \
                translations/mx-dockmaker_hi.ts \
                translations/mx-dockmaker_hr.ts \
                translations/mx-dockmaker_hu.ts \
                translations/mx-dockmaker_id.ts \
                translations/mx-dockmaker_is.ts \
                translations/mx-dockmaker_it.ts \
                translations/mx-dockmaker_ja.ts \
                translations/mx-dockmaker_kk.ts \
                translations/mx-dockmaker_ko.ts \
                translations/mx-dockmaker_lt.ts \
                translations/mx-dockmaker_mk.ts \
                translations/mx-dockmaker_mr.ts \
                translations/mx-dockmaker_nb.ts \
                translations/mx-dockmaker_nl.ts \
                translations/mx-dockmaker_pl.ts \
                translations/mx-dockmaker_pt_BR.ts \
                translations/mx-dockmaker_pt.ts \
                translations/mx-dockmaker_ro.ts \
                translations/mx-dockmaker_ru.ts \
                translations/mx-dockmaker_sk.ts \
                translations/mx-dockmaker_sl.ts \
                translations/mx-dockmaker_sq.ts \
                translations/mx-dockmaker_sr.ts \
                translations/mx-dockmaker_sv.ts \
                translations/mx-dockmaker_tr.ts \
                translations/mx-dockmaker_uk.ts \
                translations/mx-dockmaker_vi.ts \
                translations/mx-dockmaker_zh_CN.ts \
                translations/mx-dockmaker_zh_TW.ts


RESOURCES += \
    images.qrc

