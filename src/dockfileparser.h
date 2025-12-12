/**********************************************************************
 *  dockfileparser.h
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

#include <QFile>
#include <QObject>
#include <QRegularExpression>
#include <QSet>
#include <QString>

#include "dockconfiguration.h"

class DockFileParser : public QObject
{
    Q_OBJECT
public:
    QString extractSlitLocation(const QString &content) const;
    QString getLastError() const;
    bool parseContent(const QString &content, DockConfiguration &configuration);
    bool parseFile(const QString &filePath, DockConfiguration &configuration);
    bool validateFile(const QString &filePath);
    explicit DockFileParser(QObject *parent = nullptr);
    static QString extractDockName(const QString &fileName);

signals:
    void parsingCompleted(bool success);
    void parsingError(const QString &error);
    void parsingStarted();

private:
    QString m_lastError; ///< Last error message

    // Parsing constants
    DockIconInfo parseWmalauncherLine(const QString &line);
    static bool isKnownOption(const QString &token);
    static const QSet<QString> KNOWN_OPTIONS;
    static const QStringList POSSIBLE_LOCATIONS;
    static QString stripQuotes(const QString &value);
    void clearLastError();
    void setLastError(const QString &error);
};
