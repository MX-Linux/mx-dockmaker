/**********************************************************************
 *  dockconfiguration.h
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

#include <QList>
#include <QObject>
#include <QString>

#include "dockiconinfo.h"

/**
 * @brief Manages dock configuration data and metadata
 * 
 * This class serves as the data model for dock configurations,
 * managing the list of applications and dock metadata.
 */
class DockConfiguration : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit DockConfiguration(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~DockConfiguration() = default;

    // Application management
    /**
     * @brief Add an application to the dock
     * @param iconInfo Information about the icon to add
     * @return Index where the icon was added, -1 on failure
     */
    int addApplication(const DockIconInfo &iconInfo);

    /**
     * @brief Remove an application from the dock
     * @param index Index of the application to remove
     * @return true if successful, false on failure
     */
    bool removeApplication(int index);

    /**
     * @brief Clear all applications from the dock
     */
    void clearApplications();

    /**
     * @brief Update an application in the dock
     * @param index Index of the application to update
     * @param iconInfo New icon information
     * @return true if successful, false on failure
     */
    bool updateApplication(int index, const DockIconInfo &iconInfo);

    /**
     * @brief Get application information
     * @param index Index of the application
     * @return DockIconInfo for the application
     */
    DockIconInfo getApplication(int index) const;

    /**
     * @brief Get all applications
     * @return List of all dock applications
     */
    QList<DockIconInfo> getApplications() const;

    /**
     * @brief Get number of applications
     * @return Number of applications in the dock
     */
    int getApplicationCount() const;

    /**
     * @brief Check if dock has any applications
     * @return true if dock is empty
     */
    bool isEmpty() const;

    /**
     * @brief Move an application to a new position
     * @param fromIndex Current index of the application
     * @param toIndex Target index for the application
     * @return true if successful, false on failure
     */
    bool moveApplication(int fromIndex, int toIndex);

    /**
     * @brief Swap two applications
     * @param index1 Index of first application
     * @param index2 Index of second application
     * @return true if successful, false on failure
     */
    bool swapApplications(int index1, int index2);

    // Dock metadata
    /**
     * @brief Set the dock name
     * @param name Name to set
     */
    void setDockName(const QString &name);

    /**
     * @brief Get the dock name
     * @return Current dock name
     */
    QString getDockName() const;

    /**
     * @brief Set the slit location
     * @param location Location string (e.g., "BottomCenter")
     */
    void setSlitLocation(const QString &location);

    /**
     * @brief Get the slit location
     * @return Current slit location
     */
    QString getSlitLocation() const;

    /**
     * @brief Set the file name
     * @param fileName File name to set
     */
    void setFileName(const QString &fileName);

    /**
     * @brief Get the file name
     * @return Current file name
     */
    QString getFileName() const;

    // State management
    /**
     * @brief Clear all applications and reset state
     */
    void clear();

    /**
     * @brief Check if configuration has been modified
     * @return true if configuration has unsaved changes
     */
    bool isModified() const;

    /**
     * @brief Mark configuration as saved (reset modified flag)
     */
    void markAsSaved();

    /**
     * @brief Validate the configuration
     * @return true if configuration is valid
     */
    bool isValid() const;

    // Legacy compatibility
    /**
     * @brief Convert to legacy string list format
     * @return List of string lists for each application
     */
    QList<QStringList> toLegacyFormat() const;

    /**
     * @brief Load from legacy string list format
     * @param legacyData List of string lists for each application
     */
    void fromLegacyFormat(const QList<QStringList> &legacyData);

signals:
    /**
     * @brief Emitted when an application is added
     * @param index Index where the application was added
     */
    void applicationAdded(int index);

    /**
     * @brief Emitted when an application is removed
     * @param index Index where the application was removed
     */
    void applicationRemoved(int index);

    /**
     * @brief Emitted when an application is updated
     * @param index Index of the updated application
     */
    void applicationUpdated(int index);

    /**
     * @brief Emitted when applications are reordered
     */
    void applicationsReordered();

    /**
     * @brief Emitted when configuration is modified
     */
    void configurationModified();

    /**
     * @brief Emitted when configuration is saved
     */
    void configurationSaved();

private:
    QList<DockIconInfo> m_applications;  ///< List of dock applications
    QString m_dockName;                 ///< Name of the dock
    QString m_slitLocation;              ///< Location of the slit
    QString m_fileName;                 ///< File name for the dock
    bool m_modified;                    ///< Track if configuration has been modified

    /**
     * @brief Validate index is within bounds
     * @param index Index to validate
     * @return true if index is valid
     */
    bool isValidIndex(int index) const;
};

