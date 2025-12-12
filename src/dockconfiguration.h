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

class DockConfiguration : public QObject
{
    Q_OBJECT
public:
    explicit DockConfiguration(QObject *parent = nullptr);
    ~DockConfiguration() = default;

    // Application management
    DockIconInfo getApplication(int index) const;
    QList<DockIconInfo> getApplications() const;
    bool insertApplication(int index, const DockIconInfo &iconInfo, bool validate = true);
    bool isEmpty() const;
    bool moveApplication(int fromIndex, int toIndex);
    bool removeApplication(int index);
    bool swapApplications(int index1, int index2);
    bool updateApplication(int index, const DockIconInfo &iconInfo);
    int addApplication(const DockIconInfo &iconInfo, bool validate = true);
    int getApplicationCount() const;
    void clearApplications();

    // Dock metadata
    QString getDockName() const;
    QString getFileName() const;
    QString getSlitLocation() const;
    void setDockName(const QString &name);
    void setFileName(const QString &fileName);
    void setSlitLocation(const QString &location);

    // State management
    bool isModified() const;
    bool isValid() const;
    void clear();
    void markAsSaved();

    // Legacy compatibility
    QList<QStringList> toLegacyFormat() const;
    void fromLegacyFormat(const QList<QStringList> &legacyData);

signals:
    void applicationAdded(int index);
    void applicationRemoved(int index);
    void applicationUpdated(int index);
    void applicationsReordered();
    void configurationModified();
    void configurationSaved();

private:
    QList<DockIconInfo> m_applications; ///< List of dock applications
    QString m_dockName;                 ///< Name of the dock
    QString m_fileName;                 ///< File name for the dock
    QString m_slitLocation;             ///< Location of the slit
    bool isValidIndex(int index) const;
    bool m_modified; ///< Track if configuration has been modified
};
