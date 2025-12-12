/**********************************************************************
 *  icondragdrophandler.cpp
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

#include "icondragdrophandler.h"

#include <QApplication>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QWidget>

IconDragDropHandler::IconDragDropHandler(QObject *parent)
    : QObject(parent),
      m_dragging(false),
      m_dragStartIndex(-1),
      m_parentWidget(nullptr),
      m_dragIndicator(nullptr)
{
}

IconDragDropHandler::~IconDragDropHandler()
{
    cleanup();
}

void IconDragDropHandler::setIconLabels(const QList<QLabel *> &iconLabels)
{
    m_iconLabels = iconLabels;
    cleanupInsertionIndicators();
    createInsertionIndicators();
}

int IconDragDropHandler::handleMousePress(QMouseEvent *event, QWidget *clickedWidget, const QList<QLabel *> &iconLabels)
{
    if (!clickedWidget || event->button() != Qt::LeftButton) {
        return -1;
    }

    // Clean up any existing drag state
    cleanupDragIndicator();
    cleanupInsertionIndicators();
    m_dragging = false;
    m_dragStartIndex = -1;

    // Check if this is an icon label
    if (!isIconLabel(clickedWidget, iconLabels)) {
        return -1;
    }

    // Find the index of the clicked icon
    int index = getIconLabelIndex(qobject_cast<QLabel *>(clickedWidget));
    if (index >= 0) {
        m_dragStartIndex = index;
        m_dragStartPos = event->pos();
        m_dragging = false; // Will become true on mouse move
    }

    return index;
}

bool IconDragDropHandler::handleMouseMove(QMouseEvent *event, QWidget *parentWidget)
{
    if (m_dragStartIndex < 0) {
        return false;
    }

    if (!m_dragging) {
        // Check if we've moved enough to start dragging
        QPoint delta = event->pos() - m_dragStartPos;
        if (delta.manhattanLength() > QApplication::startDragDistance()) {
            m_dragging = true;
            m_parentWidget = parentWidget;
            emit dragStarted(m_dragStartIndex);

            if (m_dragStartIndex < m_iconLabels.size()) {
                QLabel *sourceIcon = m_iconLabels.at(m_dragStartIndex);
                createDragIndicator(sourceIcon, parentWidget);
                createInsertionIndicators();
            }
        }
    }

    // Update drag indicator position and insertion indicators
    if (m_dragging && m_dragIndicator) {
        m_dragIndicator->move(event->pos() - QPoint(m_dragIndicator->width() / 2, m_dragIndicator->height() / 2));
        updateInsertionIndicators(event->pos());
    }

    return m_dragging;
}

int IconDragDropHandler::handleMouseRelease(QMouseEvent *event)
{
    if (!m_dragging || m_dragStartIndex < 0) {
        return -1;
    }

    int closestIndex = findClosestInsertionPoint(event->pos());
    int targetIndex = -1;

    if (closestIndex >= 0 && closestIndex != m_dragStartIndex) {
        // Adjust insertion index for drag and drop reordering
        targetIndex = closestIndex;
        if (targetIndex >= m_iconLabels.size()) {
            // Dropping after last item
            targetIndex = m_iconLabels.size() - 1;
        } else if (targetIndex > m_dragStartIndex) {
            targetIndex--; // Account for the removed item
        }

        // Ensure targetIndex is within bounds
        if (targetIndex >= 0 && targetIndex < m_iconLabels.size()) {
            emit dragEnded(m_dragStartIndex, targetIndex);
        }
    }

    // Clean up
    cleanupDragIndicator();
    cleanupInsertionIndicators();
    m_dragging = false;
    m_dragStartIndex = -1;
    m_parentWidget = nullptr;

    return targetIndex;
}

void IconDragDropHandler::handleResizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)

    // Reposition insertion indicators when window is resized
    if (!m_insertionIndicators.isEmpty()) {
        positionInsertionIndicators();
    }
}

bool IconDragDropHandler::isDragging() const
{
    return m_dragging;
}

int IconDragDropHandler::getDragStartIndex() const
{
    return m_dragStartIndex;
}

void IconDragDropHandler::resetDragState()
{
    cleanupDragIndicator();
    cleanupInsertionIndicators();
    m_dragging = false;
    m_dragStartIndex = -1;
    m_dragStartPos = QPoint();
}

void IconDragDropHandler::cleanup()
{
    cleanupDragIndicator();
    cleanupInsertionIndicators();
    m_iconLabels.clear();
    m_dragging = false;
    m_dragStartIndex = -1;
    m_dragStartPos = QPoint();
    m_parentWidget = nullptr;
}

void IconDragDropHandler::createInsertionIndicators()
{
    // Clear any existing indicators
    cleanupInsertionIndicators();

    if (m_iconLabels.isEmpty() || !m_parentWidget) {
        return;
    }

    // Get icon size from the first icon
    QSize iconSize = m_iconLabels.first()->size();

    // Create insertion points: before first, between each pair, and after last
    int numIndicators = m_iconLabels.size() + 1;

    for (int i = 0; i < numIndicators; ++i) {
        QLabel *indicator = new QLabel(m_parentWidget);
        indicator->setFixedSize(iconSize);
        indicator->setStyleSheet("background: rgba(0, 120, 212, 180); border: 2px solid #0078d4; border-radius: 4px;");
        indicator->setAttribute(Qt::WA_TransparentForMouseEvents);
        indicator->hide(); // Initially hidden
        m_insertionIndicators.append(indicator);
    }

    // Position the indicators
    positionInsertionIndicators();
}

void IconDragDropHandler::positionInsertionIndicators()
{
    if (m_iconLabels.isEmpty() || m_insertionIndicators.size() != m_iconLabels.size() + 1 || !m_parentWidget) {
        return;
    }

    // Position indicators: before first icon, between icons, and after last icon
    for (int i = 0; i < m_insertionIndicators.size(); ++i) {
        QLabel *indicator = m_insertionIndicators.at(i);

        if (i == 0) {
            // Before first icon
            QLabel *firstIcon = m_iconLabels.first();
            QPoint iconPos = firstIcon->mapTo(m_parentWidget, QPoint(0, 0));
            int centerY = iconPos.y() + firstIcon->height() / 2 - indicator->height() / 2;
            QPoint pos(iconPos.x() - indicator->width() - 5, centerY);
            indicator->move(pos);
        } else if (i == m_insertionIndicators.size() - 1) {
            // After last icon
            QLabel *lastIcon = m_iconLabels.last();
            QPoint iconPos = lastIcon->mapTo(m_parentWidget, QPoint(lastIcon->width(), 0));
            int centerY = iconPos.y() + lastIcon->height() / 2 - indicator->height() / 2;
            QPoint pos(iconPos.x() + 5, centerY);
            indicator->move(pos);
        } else {
            // Between icons i-1 and i
            if (i - 1 < m_iconLabels.size() && i < m_iconLabels.size()) {
                QLabel *leftIcon = m_iconLabels.at(i - 1);
                QLabel *rightIcon = m_iconLabels.at(i);
                QPoint leftPos = leftIcon->mapTo(m_parentWidget, QPoint(leftIcon->width(), 0));
                QPoint rightPos = rightIcon->mapTo(m_parentWidget, QPoint(0, 0));
                int centerX = (leftPos.x() + rightPos.x()) / 2;
                int centerY = leftPos.y() + leftIcon->height() / 2 - indicator->height() / 2;
                QPoint pos(centerX - indicator->width() / 2, centerY);
                indicator->move(pos);
            }
        }
    }
}

void IconDragDropHandler::updateInsertionIndicators(QPoint mousePos)
{
    if (m_insertionIndicators.isEmpty()) {
        return;
    }

    // Hide all indicators first
    for (int i = 0; i < m_insertionIndicators.size(); ++i) {
        m_insertionIndicators.at(i)->hide();
    }

    // Find which insertion point is closest to the mouse
    int closestIndex = -1;
    int minDistance = INT_MAX;

    for (int i = 0; i < m_insertionIndicators.size(); ++i) {
        QLabel *indicator = m_insertionIndicators.at(i);
        QPoint indicatorPos = indicator->pos() + QPoint(indicator->width() / 2, indicator->height() / 2);
        int distance = QPoint(mousePos - indicatorPos).manhattanLength();

        if (distance < minDistance) {
            minDistance = distance;
            closestIndex = i;
        }
    }

    // Show only the closest indicator if it's within a reasonable distance
    if (closestIndex >= 0 && minDistance < DROP_THRESHOLD) {
        m_insertionIndicators.at(closestIndex)->show();
        m_insertionIndicators.at(closestIndex)->raise();
    }
}

void IconDragDropHandler::createDragIndicator(QLabel *sourceIcon, QWidget *parentWidget)
{
    cleanupDragIndicator();

    if (!sourceIcon || !parentWidget) {
        return;
    }

    m_dragIndicator = new QLabel(parentWidget);
    m_dragIndicator->setPixmap(sourceIcon->pixmap(Qt::ReturnByValue));
    m_dragIndicator->setAlignment(Qt::AlignCenter);
    m_dragIndicator->setFixedSize(sourceIcon->size());
    m_dragIndicator->setStyleSheet("background: rgba(255, 255, 255, 128); border: 2px dashed #0078d4; opacity: 0.8;");
    m_dragIndicator->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_dragIndicator->raise(); // Make sure it's on top
    m_dragIndicator->show();

    // Make source icon semi-transparent
    QString originalStyle = sourceIcon->styleSheet();
    sourceIcon->setStyleSheet(originalStyle + "opacity: 0.5;");
    sourceIcon->setProperty("original_style", originalStyle);
}

int IconDragDropHandler::findClosestInsertionPoint(QPoint mousePos)
{
    if (m_insertionIndicators.isEmpty()) {
        return -1;
    }

    int closestIndex = -1;
    int minDistance = INT_MAX;

    for (int i = 0; i < m_insertionIndicators.size(); ++i) {
        QLabel *indicator = m_insertionIndicators.at(i);
        QRect indicatorRect = indicator->geometry();
        QPoint center = indicatorRect.center();
        int distance = QPoint(mousePos - center).manhattanLength();

        if (distance < minDistance) {
            minDistance = distance;
            closestIndex = i;
        }
    }

    return closestIndex;
}

bool IconDragDropHandler::isIconLabel(QWidget *widget, const QList<QLabel *> &iconLabels) const
{
    QLabel *label = qobject_cast<QLabel *>(widget);
    if (!label) {
        return false;
    }

    return iconLabels.contains(label);
}

int IconDragDropHandler::getIconLabelIndex(QLabel *label) const
{
    return m_iconLabels.indexOf(label);
}

void IconDragDropHandler::cleanupDragIndicator()
{
    if (m_dragIndicator) {
        // Use deleteLater() to avoid double-deletion if parent widget was already destroyed
        m_dragIndicator->deleteLater();
        m_dragIndicator = nullptr;
    }

    // Restore original style of source icon
    if (m_dragStartIndex >= 0 && m_dragStartIndex < m_iconLabels.size()) {
        QLabel *sourceIcon = m_iconLabels.at(m_dragStartIndex);
        QString originalStyle = sourceIcon->property("original_style").toString();
        if (!originalStyle.isEmpty()) {
            sourceIcon->setStyleSheet(originalStyle);
            sourceIcon->setProperty("original_style", QVariant());
        }
    }
}

void IconDragDropHandler::cleanupInsertionIndicators()
{
    // Use deleteLater() to safely schedule deletion and avoid double-deletion
    // if parent widget was already destroyed
    for (QLabel *indicator : m_insertionIndicators) {
        if (indicator) {
            indicator->deleteLater();
        }
    }
    m_insertionIndicators.clear();
}
