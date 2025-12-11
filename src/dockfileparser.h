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

/**
 * @brief Handles parsing of dock configuration files
 * 
 * This class is responsible for reading and parsing .mxdk dock files,
 * extracting application configurations and slit location information.
 */
class DockFileParser : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit DockFileParser(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~DockFileParser() = default;

    /**
     * @brief Parse a dock file and populate configuration
     * @param filePath Path to the dock file to parse
     * @param configuration Configuration object to populate
     * @return true if parsing was successful, false on error
     */
    bool parseFile(const QString &filePath, DockConfiguration &configuration);

    /**
     * @brief Parse dock file content from string
     * @param content File content as string
     * @param configuration Configuration object to populate
     * @return true if parsing was successful, false on error
     */
    bool parseContent(const QString &content, DockConfiguration &configuration);

    /**
     * @brief Extract slit location from file content
     * @param content File content to search
     * @return Slit location string, empty if not found
     */
    QString extractSlitLocation(const QString &content);

    /**
     * @brief Extract dock name from file name
     * @param fileName File name to extract dock name from
     * @return Dock name, empty if not found
     */
    static QString extractDockName(const QString &fileName);

    /**
     * @brief Validate dock file format
     * @param filePath Path to the dock file
     * @return true if file format is valid, false otherwise
     */
    bool validateFile(const QString &filePath);

    /**
     * @brief Get last error message
     * @return Error description from last parsing operation
     */
    QString getLastError() const;

signals:
    /**
     * @brief Emitted when parsing starts
     */
    void parsingStarted();

    /**
     * @brief Emitted when parsing completes
     * @param success true if parsing was successful
     */
    void parsingCompleted(bool success);

    /**
     * @brief Emitted when an error occurs during parsing
     * @param error Error description
     */
    void parsingError(const QString &error);

private:
    QString m_lastError;  ///< Last error message

    // Parsing constants
    static const QStringList POSSIBLE_LOCATIONS;
    static const QSet<QString> KNOWN_OPTIONS;

    /**
     * @brief Parse a single wmalauncher command line
     * @param line Command line to parse
     * @return DockIconInfo parsed from the line
     */
    DockIconInfo parseWmalauncherLine(const QString &line);

    /**
     * @brief Strip quotes from a string value
     * @param value String to strip quotes from
     * @return String without surrounding quotes
     */
    static QString stripQuotes(const QString &value);

    /**
     * @brief Check if a token is a known option
     * @param token Token to check
     * @return true if token is a known option
     */
    static bool isKnownOption(const QString &token);

    /**
     * @brief Set last error message
     * @param error Error message to set
     */
    void setLastError(const QString &error);

    /**
     * @brief Clear last error message
     */
    void clearLastError();
};

