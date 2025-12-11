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

#include <QFile>
#include <QObject>
#include <QString>

#include "cmd.h"
#include "dockconfiguration.h"

/**
 * @brief Handles file I/O operations for dock configurations
 * 
 * This class is responsible for reading, writing, and managing dock files,
 * including backup creation, permission management, and Fluxbox menu integration.
 */
class DockFileManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit DockFileManager(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~DockFileManager() = default;

    /**
     * @brief Save configuration to dock file
     * @param configuration Configuration to save
     * @param filePath Target file path
     * @param createBackup Whether to create a backup file
     * @return true if save was successful, false on error
     */
    bool saveConfiguration(const DockConfiguration &configuration, 
                           const QString &filePath, 
                           bool createBackup = true);

    /**
     * @brief Load configuration from dock file
     * @param filePath Source file path
     * @param configuration Configuration to populate
     * @return true if load was successful, false on error
     */
    bool loadConfiguration(const QString &filePath, DockConfiguration &configuration);

    /**
     * @brief Delete a dock file
     * @param filePath File path to delete
     * @param removeFromMenu Whether to remove from Fluxbox menu
     * @return true if deletion was successful, false on error
     */
    bool deleteDockFile(const QString &filePath, bool removeFromMenu = true);

    /**
     * @brief Move a dock file to a new location
     * @param oldFilePath Current file path
     * @param newSlitLocation New slit location
     * @return true if move was successful, false on error
     */
    bool moveDockFile(const QString &oldFilePath, const QString &newSlitLocation);

    /**
     * @brief Add dock to Fluxbox menu
     * @param filePath Path to the dock file
     * @param dockName Name to display in menu
     * @return true if addition was successful, false on error
     */
    bool addToMenu(const QString &filePath, const QString &dockName);

    /**
     * @brief Remove dock from Fluxbox menu
     * @param filePath Path to the dock file
     * @return true if removal was successful, false on error
     */
    bool removeFromMenu(const QString &filePath);

    /**
     * @brief Check if dock is in Fluxbox menu
     * @param filePath Path to the dock file
     * @return true if dock is in menu
     */
    bool isInMenu(const QString &filePath);

    /**
     * @brief Create backup of a file
     * @param filePath File to backup
     * @return true if backup was created successfully
     */
    bool createBackup(const QString &filePath);

    /**
     * @brief Set executable permissions on dock file
     * @param filePath File path to make executable
     * @return true if permissions were set successfully
     */
    bool setExecutable(const QString &filePath);

    /**
     * @brief Ensure scripts directory exists
     * @return true if directory exists or was created successfully
     */
    bool ensureScriptsDirectory();

    /**
     * @brief Get default dock file directory
     * @return Path to default dock directory
     */
    static QString getDefaultDockDirectory();

    /**
     * @brief Get last error message
     * @return Error description from last operation
     */
    QString getLastError() const;

signals:
    /**
     * @brief Emitted when file operation starts
     * @param operation Description of the operation
     */
    void operationStarted(const QString &operation);

    /**
     * @brief Emitted when file operation completes
     * @param operation Description of the operation
     * @param success true if operation was successful
     */
    void operationCompleted(const QString &operation, bool success);

    /**
     * @brief Emitted when an error occurs during file operation
     * @param operation Description of the operation
     * @param error Error description
     */
    void operationError(const QString &operation, const QString &error);

private:
    Cmd m_cmd;           ///< Command execution helper
    QString m_lastError; ///< Last error message

    /**
     * @brief Generate dock file content from configuration
     * @param configuration Configuration to convert
     * @return String content for dock file
     */
    QString generateDockContent(const DockConfiguration &configuration);

    /**
     * @brief Securely escape string for shell commands
     * @param arg String to escape
     * @return Escaped string safe for shell use
     */
    static QString escapeShellArg(const QString &arg);

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

