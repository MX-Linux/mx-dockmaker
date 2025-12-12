/**********************************************************************
 *  icondragdrophandler.h
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

#include <QLabel>
#include <QObject>
#include <QPoint>
#include <QSize>

class IconDragDropHandler : public QObject
{
    Q_OBJECT
public:
    bool handleMouseMove(QMouseEvent *event, QWidget *parentWidget);
    bool isDragging() const;
    explicit IconDragDropHandler(QObject *parent = nullptr);
    int getDragStartIndex() const;
    int handleMousePress(QMouseEvent *event, QWidget *clickedWidget, const QList<QLabel *> &iconLabels);
    int handleMouseRelease(QMouseEvent *event);
    void cleanup();
    void handleResizeEvent(QResizeEvent *event);
    void resetDragState();
    void setIconLabels(const QList<QLabel *> &iconLabels);
    ~IconDragDropHandler();

signals:
    void dragEnded(int fromIndex, int toIndex);
    void dragStarted(int index);

private:
    QLabel *m_dragIndicator;               ///< Visual drag indicator
    QList<QLabel *> m_iconLabels;          ///< List of icon labels
    QList<QLabel *> m_insertionIndicators; ///< Insertion point indicators
    QPoint m_dragStartPos;                 ///< Position where drag started
    QWidget *m_parentWidget;               ///< Widget receiving mouse events
    bool m_dragging;                       ///< Whether drag is in progress
    int m_dragStartIndex;                  ///< Index where drag started

    // Visual constants
    bool isIconLabel(QWidget *widget, const QList<QLabel *> &iconLabels) const;
    int findClosestInsertionPoint(QPoint mousePos);
    int getIconLabelIndex(QLabel *label) const;
    static constexpr int DRAG_THRESHOLD = 10;  ///< Minimum pixels to start drag
    static constexpr int DROP_THRESHOLD = 200; ///< Maximum pixels for valid drop
    void cleanupDragIndicator();
    void cleanupInsertionIndicators();
    void createDragIndicator(QLabel *sourceIcon, QWidget *parentWidget);
    void createInsertionIndicators();
    void positionInsertionIndicators();
    void updateInsertionIndicators(QPoint mousePos);
};
