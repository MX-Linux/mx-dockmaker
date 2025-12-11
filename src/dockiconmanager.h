/**********************************************************************
 *  dockiconmanager.h
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

#include <QColor>
#include <QLabel>
#include <QObject>
#include <QPixmap>
#include <QSize>

#include "cmd.h"
#include "dockiconinfo.h"

/**
 * @brief Manages icon operations and styling for dock applications
 * 
 * This class handles finding, loading, and displaying icons, as well as
 * applying visual styles to dock icons.
 */
class DockIconManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit DockIconManager(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~DockIconManager() = default;

    /**
     * @brief Find and load an icon
     * @param iconName Name or path of the icon
     * @param size Desired size of the icon
     * @return QPixmap containing the icon, empty if not found
     */
    QPixmap findIcon(const QString &iconName, const QSize &size);

    /**
     * @brief Display an icon in a label
     * @param iconInfo Information about the icon to display
     * @param label Label to display the icon in
     * @param selectedIndex Currently selected index (for styling)
     * @param currentIndex Index of this icon
     */
    void displayIcon(const DockIconInfo &iconInfo, QLabel *label, 
                   int selectedIndex = -1, int currentIndex = -1);

    /**
     * @brief Apply visual styles to an icon label
     * @param iconInfo Information about the icon
     * @param label Label to style
     * @param isSelected Whether this icon is currently selected
     */
    void applyIconStyle(const DockIconInfo &iconInfo, QLabel *label, bool isSelected = false);

    /**
     * @brief Get icon container size including padding and borders
     * @param iconSize Size of the icon itself
     * @return Total size including padding and borders
     */
    static QSize getIconContainerSize(const QSize &iconSize);

    /**
     * @brief Get default icon size
     * @return Default icon size as string (e.g., "48x48")
     */
    static QString getDefaultIconSize();

    /**
     * @brief Get available icon sizes
     * @return List of available icon size strings
     */
    static QStringList getAvailableIconSizes();

    /**
     * @brief Get last error message
     * @return Error description from last operation
     */
    QString getLastError() const;

signals:
    /**
     * @brief Emitted when icon search starts
     * @param iconName Name of the icon being searched for
     */
    void iconSearchStarted(const QString &iconName);

    /**
     * @brief Emitted when icon search completes
     * @param iconName Name of the icon that was searched for
     * @param found true if the icon was found
     */
    void iconSearchCompleted(const QString &iconName, bool found);

    /**
     * @brief Emitted when an error occurs during icon operation
     * @param operation Description of the operation
     * @param error Error description
     */
    void iconError(const QString &operation, const QString &error);

private:
    Cmd m_cmd;           ///< Command execution helper
    QString m_lastError; ///< Last error message

    // Icon styling constants
    static constexpr int ICON_BORDER_WIDTH = 4;
    static constexpr int ICON_PADDING = 4;

    /**
     * @brief Search for icon in standard locations
     * @param iconName Name of the icon to search for
     * @param size Desired size of the icon
     * @return QPixmap containing the icon, empty if not found
     */
    QPixmap searchIconInPaths(const QString &iconName, const QSize &size);

    /**
     * @brief Try to find icon from theme
     * @param iconName Name of the icon
     * @param size Desired size of the icon
     * @return QPixmap containing the icon, empty if not found
     */
    QPixmap findThemeIcon(const QString &iconName, const QSize &size);

    /**
     * @brief Try to find icon in filesystem paths
     * @param iconName Name of the icon
     * @param size Desired size of the icon
     * @return QPixmap containing the icon, empty if not found
     */
    QPixmap findFilesystemIcon(const QString &iconName, const QSize &size);

    /**
     * @brief Get standard icon search paths
     * @return List of paths to search for icons
     */
    QStringList getIconSearchPaths();

    /**
     * @brief Generate style sheet for icon
     * @param iconInfo Information about the icon
     * @param isSelected Whether this icon is currently selected
     * @return CSS style string
     */
    QString generateIconStyle(const DockIconInfo &iconInfo, bool isSelected);

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

