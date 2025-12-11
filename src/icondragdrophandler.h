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

#ifndef ICONDRAGDROPHANDLER_H
#define ICONDRAGDROPHANDLER_H

#include <QLabel>
#include <QObject>
#include <QPoint>
#include <QSize>

/**
 * @brief Handles drag-and-drop functionality for dock icons
 * 
 * This class manages all aspects of drag-and-drop operations including
 * visual feedback, insertion indicators, and icon reordering.
 */
class IconDragDropHandler : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit IconDragDropHandler(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~IconDragDropHandler();

    /**
     * @brief Set the list of icon labels to manage
     * @param iconLabels List of icon labels
     */
    void setIconLabels(const QList<QLabel *> &iconLabels);

    /**
     * @brief Handle mouse press event
     * @param event Mouse press event
     * @param clickedWidget Widget that was clicked
     * @param iconLabels List of icon labels to check against
     * @return Index of clicked icon, -1 if no icon was clicked
     */
    int handleMousePress(QMouseEvent *event, QWidget *clickedWidget, 
                      const QList<QLabel *> &iconLabels);

    /**
     * @brief Handle mouse move event
     * @param event Mouse move event
     * @param parentWidget Parent widget for positioning
     * @return true if drag is in progress
     */
    bool handleMouseMove(QMouseEvent *event, QWidget *parentWidget);

    /**
     * @brief Handle mouse release event
     * @param event Mouse release event
     * @return Index where icon was dropped, -1 if no valid drop
     */
    int handleMouseRelease(QMouseEvent *event);

    /**
     * @brief Handle widget resize event
     * @param event Resize event
     */
    void handleResizeEvent(QResizeEvent *event);

    /**
     * @brief Check if drag operation is in progress
     * @return true if currently dragging
     */
    bool isDragging() const;

    /**
     * @brief Get the index of the icon being dragged
     * @return Drag start index, -1 if not dragging
     */
    int getDragStartIndex() const;

    /**
     * @brief Reset drag state
     */
    void resetDragState();

    /**
     * @brief Clean up all drag-related resources
     */
    void cleanup();

signals:
    /**
     * @brief Emitted when drag operation starts
     * @param index Index of the icon being dragged
     */
    void dragStarted(int index);

    /**
     * @brief Emitted when drag operation ends
     * @param fromIndex Original index of the dragged icon
     * @param toIndex Target index where icon was dropped
     */
    void dragEnded(int fromIndex, int toIndex);

    /**
     * @brief Emitted when an icon is clicked (not dragged)
     * @param index Index of the clicked icon
     */
    void iconClicked(int index);

private:
    QList<QLabel *> m_iconLabels;      ///< List of icon labels
    bool m_dragging;                  ///< Whether drag is in progress
    int m_dragStartIndex;              ///< Index where drag started
    QPoint m_dragStartPos;             ///< Position where drag started
    QLabel *m_dragIndicator;           ///< Visual drag indicator
    QList<QLabel *> m_insertionIndicators; ///< Insertion point indicators

    // Visual constants
    static constexpr int DRAG_THRESHOLD = 10;  ///< Minimum pixels to start drag
    static constexpr int DROP_THRESHOLD = 200;  ///< Maximum pixels for valid drop

    /**
     * @brief Create insertion indicators
     */
    void createInsertionIndicators();

    /**
     * @brief Position insertion indicators
     */
    void positionInsertionIndicators();

    /**
     * @brief Update insertion indicators based on mouse position
     * @param mousePos Current mouse position
     */
    void updateInsertionIndicators(const QPoint &mousePos);

    /**
     * @brief Create drag indicator
     * @param sourceIcon Icon being dragged
     * @param parentWidget Parent widget for positioning
     */
    void createDragIndicator(QLabel *sourceIcon, QWidget *parentWidget);

    /**
     * @brief Find the closest insertion point to mouse position
     * @param mousePos Current mouse position
     * @return Index of closest insertion point
     */
    int findClosestInsertionPoint(const QPoint &mousePos);

    /**
     * @brief Check if a widget is an icon label
     * @param widget Widget to check
     * @param iconLabels List of icon labels
     * @return true if widget is an icon label
     */
    bool isIconLabel(QWidget *widget, const QList<QLabel *> &iconLabels);

    /**
     * @brief Get the index of an icon label
     * @param label Label to find
     * @return Index of the label, -1 if not found
     */
    int getIconLabelIndex(QLabel *label);

    /**
     * @brief Clean up drag indicator
     */
    void cleanupDragIndicator();

    /**
     * @brief Clean up insertion indicators
     */
    void cleanupInsertionIndicators();
};

#endif // ICONDRAGDROPHANDLER_H