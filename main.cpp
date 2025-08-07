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
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QProcess>
#include <QTranslator>

#include "mainwindow.h"
#include <unistd.h>

int main(int argc, char *argv[])
{
    // Set Qt platform to XCB (X11) if not already set and we're in X11 environment
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        if (!qEnvironmentVariableIsEmpty("DISPLAY") && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")) {
            qputenv("QT_QPA_PLATFORM", "xcb");
        }
    }

    QApplication a(argc, argv);
    QApplication::setWindowIcon(QIcon("/usr/share/icons/hicolor/192x192/apps/mx-dockmaker.png"));
    QApplication::setApplicationName(QStringLiteral("mx-dockmaker"));
    QApplication::setOrganizationName(QStringLiteral("MX-Linux"));

    QTranslator qtTran;
    if (qtTran.load("qt_" + QLocale().name(), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        QApplication::installTranslator(&qtTran);
    }

    QTranslator qtBaseTran;
    if (qtBaseTran.load("qtbase_" + QLocale().name(), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        QApplication::installTranslator(&qtBaseTran);
    }

    QTranslator appTran;
    if (appTran.load(QApplication::applicationName() + "_" + QLocale().name(),
                     "/usr/share/" + QApplication::applicationName() + "/locale")) {
        QApplication::installTranslator(&appTran);
    }

    // root guard
    if (QProcess::execute("/bin/bash", {"-c", "logname |grep -q ^root$"}) == 0) {
        QMessageBox::critical(
            nullptr, QObject::tr("Error"),
            QObject::tr(
                "You seem to be logged in as root, please log out and log in as normal user to use this program."));
        exit(EXIT_FAILURE);
    }

    if (getuid() != 0) {
        QString file;
        if (QApplication::arguments().length() >= 2 && QFile::exists(QApplication::arguments().at(1))) {
            file = QApplication::arguments().at(1);
        }
        MainWindow w(nullptr, file);
        w.show();
        return QApplication::exec();
    } else {
        QApplication::beep();
        QMessageBox::critical(nullptr, QObject::tr("Error"), QObject::tr("You must run this program as normal user."));
        return EXIT_FAILURE;
    }
}
