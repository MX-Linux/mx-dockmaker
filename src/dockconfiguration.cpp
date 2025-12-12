/**********************************************************************
 *  dockconfiguration.cpp
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

#include "dockconfiguration.h"

DockConfiguration::DockConfiguration(QObject *parent)
    : QObject(parent)
    , m_modified(false)
{
}

int DockConfiguration::addApplication(const DockIconInfo &iconInfo, bool validate)
{
    if (validate && !iconInfo.isValid()) {
        return -1;
    }

    m_applications.append(iconInfo);
    m_modified = true;

    emit applicationAdded(m_applications.size() - 1);
    emit configurationModified();

    return m_applications.size() - 1;
}

bool DockConfiguration::insertApplication(int index, const DockIconInfo &iconInfo, bool validate)
{
    if (validate && !iconInfo.isValid()) {
        return false;
    }

    // Clamp index to valid range [0, size()]
    // Allowing index == size() for appending
    if (index < 0 || index > m_applications.size()) {
        return false;
    }

    m_applications.insert(index, iconInfo);
    m_modified = true;

    emit applicationAdded(index);
    emit configurationModified();

    return true;
}

bool DockConfiguration::removeApplication(int index)
{
    if (!isValidIndex(index)) {
        return false;
    }

    m_applications.removeAt(index);
    m_modified = true;

    emit applicationRemoved(index);
    emit configurationModified();

    return true;
}

bool DockConfiguration::updateApplication(int index, const DockIconInfo &iconInfo)
{
    if (!isValidIndex(index) || !iconInfo.isValid()) {
        return false;
    }

    m_applications[index] = iconInfo;
    m_modified = true;

    emit applicationUpdated(index);
    emit configurationModified();

    return true;
}

DockIconInfo DockConfiguration::getApplication(int index) const
{
    if (!isValidIndex(index)) {
        return DockIconInfo();
    }

    return m_applications.at(index);
}

QList<DockIconInfo> DockConfiguration::getApplications() const
{
    return m_applications;
}

void DockConfiguration::clearApplications()
{
    m_applications.clear();
    emit configurationModified();
}

int DockConfiguration::getApplicationCount() const
{
    return m_applications.size();
}

bool DockConfiguration::isEmpty() const
{
    return m_applications.isEmpty();
}

bool DockConfiguration::moveApplication(int fromIndex, int toIndex)
{
    if (!isValidIndex(fromIndex) || !isValidIndex(toIndex) || fromIndex == toIndex) {
        return false;
    }

    DockIconInfo info = m_applications.takeAt(fromIndex);
    m_applications.insert(toIndex, info);
    m_modified = true;

    emit applicationsReordered();
    emit configurationModified();

    return true;
}

bool DockConfiguration::swapApplications(int index1, int index2)
{
    if (!isValidIndex(index1) || !isValidIndex(index2) || index1 == index2) {
        return false;
    }

    m_applications.swapItemsAt(index1, index2);
    m_modified = true;

    emit applicationsReordered();
    emit configurationModified();

    return true;
}

void DockConfiguration::setDockName(const QString &name)
{
    if (m_dockName != name) {
        m_dockName = name;
        m_modified = true;
        emit configurationModified();
    }
}

QString DockConfiguration::getDockName() const
{
    return m_dockName;
}

void DockConfiguration::setSlitLocation(const QString &location)
{
    if (m_slitLocation != location) {
        m_slitLocation = location;
        m_modified = true;
        emit configurationModified();
    }
}

QString DockConfiguration::getSlitLocation() const
{
    return m_slitLocation;
}

void DockConfiguration::setFileName(const QString &fileName)
{
    if (m_fileName != fileName) {
        m_fileName = fileName;
        m_modified = true;
        emit configurationModified();
    }
}

QString DockConfiguration::getFileName() const
{
    return m_fileName;
}

void DockConfiguration::clear()
{
    const bool wasEmpty = m_applications.isEmpty();

    m_applications.clear();
    m_dockName.clear();
    m_slitLocation.clear();
    m_fileName.clear();
    m_modified = false;

    if (!wasEmpty) {
        emit configurationModified();
    }
}

bool DockConfiguration::isModified() const
{
    return m_modified;
}

void DockConfiguration::markAsSaved()
{
    m_modified = false;
    emit configurationSaved();
}

bool DockConfiguration::isValid() const
{
    // Check if we have at least one valid application
    for (const auto &app : m_applications) {
        if (app.isValid()) {
            return true;
        }
    }
    return false;
}

QList<QStringList> DockConfiguration::toLegacyFormat() const
{
    QList<QStringList> legacyData;
    legacyData.reserve(m_applications.size());

    for (const auto &app : m_applications) {
        legacyData.append(app.toStringList());
    }

    return legacyData;
}

void DockConfiguration::fromLegacyFormat(const QList<QStringList> &legacyData)
{
    clear();
    m_applications.reserve(legacyData.size());

    for (const auto &appData : legacyData) {
        DockIconInfo info = DockIconInfo::fromStringList(appData);
        if (info.isValid()) {
            m_applications.append(info);
        }
    }

    if (!m_applications.isEmpty()) {
        m_modified = true;
        emit configurationModified();
    }
}

bool DockConfiguration::isValidIndex(int index) const
{
    return index >= 0 && index < m_applications.size();
}
