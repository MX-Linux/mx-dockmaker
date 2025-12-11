/**********************************************************************
 *  dockiconinfo.h
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

#ifndef DOCKICONINFO_H
#define DOCKICONINFO_H

#include <QColor>
#include <QSize>
#include <QString>

/**
 * @brief Structure containing all information for a single dock icon
 */
struct DockIconInfo
{
    // Application information
    QString appName;           ///< Name of the application or .desktop file
    QString command;           ///< Command to execute
    QString tooltip;           ///< Tooltip text to display
    QString customIcon;        ///< Custom icon path (empty if using theme icon)

    // Visual properties
    QString size;              ///< Icon size (e.g., "48x48")
    QColor backgroundColor;    ///< Normal background color
    QColor hoverBackground;   ///< Hover background color
    QColor borderColor;        ///< Normal border color
    QColor hoverBorder;        ///< Hover border color

    // Additional options
    QString extraOptions;      ///< Additional command-line options

    /**
     * @brief Default constructor
     */
    DockIconInfo() = default;

    /**
     * @brief Constructor with essential parameters
     * @param app Application name or .desktop file
     * @param cmd Command to execute
     */
    DockIconInfo(const QString &app, const QString &cmd)
        : appName(app)
        , command(cmd)
        , size("48x48")
        , backgroundColor(Qt::black)
        , hoverBackground(Qt::black)
        , borderColor(Qt::white)
        , hoverBorder(Qt::white)
    {}

    /**
     * @brief Check if this is a desktop file application
     * @return true if appName ends with .desktop
     */
    bool isDesktopFile() const
    {
        return appName.endsWith(QLatin1String(".desktop"));
    }

    /**
     * @brief Check if the icon info is valid (has required fields)
     * @return true if required fields are not empty
     */
    bool isValid() const
    {
        return !appName.isEmpty() && (!isDesktopFile() || !command.isEmpty());
    }

    /**
     * @brief Get the icon size as QSize
     * @return QSize object representing the icon size
     */
    QSize iconSize() const
    {
        const quint8 width = size.section(QLatin1Char('x'), 0, 0).toUShort();
        return QSize(width, width);
    }

    /**
     * @brief Convert to string list for legacy compatibility
     * @return String list with all fields in order
     */
    QStringList toStringList() const
    {
        return {
            appName,
            command,
            tooltip,
            customIcon,
            size,
            backgroundColor.name(),
            hoverBackground.name(),
            borderColor.name(),
            hoverBorder.name(),
            extraOptions
        };
    }

    /**
     * @brief Create from string list for legacy compatibility
     * @param list String list with all fields in order
     * @return DockIconInfo object
     */
    static DockIconInfo fromStringList(const QStringList &list)
    {
        DockIconInfo info;
        if (list.size() >= 11) {
            info.appName = list.at(0);
            info.command = list.at(1);
            info.tooltip = list.at(2);
            info.customIcon = list.at(3);
            info.size = list.at(4);
            info.backgroundColor = QColor(list.at(5));
            info.hoverBackground = QColor(list.at(6));
            info.borderColor = QColor(list.at(7));
            info.hoverBorder = QColor(list.at(8));
            info.extraOptions = list.at(9);
        }
        return info;
    }
};

#endif // DOCKICONINFO_H