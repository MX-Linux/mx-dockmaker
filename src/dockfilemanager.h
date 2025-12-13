/**********************************************************************
 *  dockfilemanager.h
 **********************************************************************
 * Copyright (C) 2017-2025 MX Authors
 *
 * Authors: Adrian
 *          MX Linux <http://mxlinux.org>
 *
 * This file is part of mx-dockmaker.
 *
 * mx-dockmaker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mx-dockmaker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mx-dockmaker.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#pragma once

#include "dockconfiguration.h"
#include <QFile>
#include <QObject>
#include <QProcess>
#include <QString>

class DockFileManager : public QObject
{
    Q_OBJECT
public:
    QString getLastError() const;
    bool addToMenu(const QString &filePath, const QString &dockName);
    bool createBackup(const QString &filePath);
    bool deleteDockFile(const QString &filePath, bool removeFromMenu = true);
    bool ensureScriptsDirectory();
    bool isInMenu(const QString &filePath) const;
    bool loadConfiguration(const QString &filePath, DockConfiguration &configuration);
    bool moveDockFile(const QString &oldFilePath, const QString &newSlitLocation);
    bool removeFromMenu(const QString &filePath);
    bool saveConfiguration(const DockConfiguration &configuration, const QString &filePath, bool createBackup = true);
    bool setExecutable(const QString &filePath);
    explicit DockFileManager(QObject *parent = nullptr);
    static QString getDefaultDockDirectory();
    static void killAndWaitForProcess(const QString &processName);

signals:
    void operationCompleted(const QString &operation, bool success);
    void operationError(const QString &operation, const QString &error);
    void operationStarted(const QString &operation);

private:
    QString m_lastError; ///< Last error message

    QString generateDockContent(const DockConfiguration &configuration) const;
    static QString escapeShellArg(const QString &arg);
    static QString escapeSedArg(const QString &arg);
    static bool runCommandQuiet(const QString &command, const QStringList &args = {});
    void clearLastError();
    void setLastError(const QString &error);
};
