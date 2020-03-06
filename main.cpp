/**********************************************************************
 *  main.cpp
 **********************************************************************
 * Copyright (C) 2020 MX Authors
 *
 * Authors: Adrian
 *          MX Linux <http://mxlinux.org>
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this package. If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QIcon>

#include "mainwindow.h"
#include <unistd.h>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon("/usr/share/icons/hicolor/192x192/apps/mx-dockmaker.png"));
    a.setApplicationName("mx-dockmaker");

    QTranslator qtTran;
    qtTran.load(QString("qt_") + QLocale::system().name());
    a.installTranslator(&qtTran);

    QTranslator appTran;
    appTran.load(QString("mx-dockmaker_") + QLocale::system().name(), "/usr/share/mx-dockmaker/locale");
    a.installTranslator(&appTran);

    if (getuid() != 0) {
        MainWindow w;
        w.show();
        return a.exec();
    } else {
        QApplication::beep();
        QMessageBox::critical(nullptr, QApplication::tr("Error"),
                              QApplication::tr("You must run this program as normal user."));
        return EXIT_FAILURE;
    }
}
