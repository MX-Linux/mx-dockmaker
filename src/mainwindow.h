/**********************************************************************
 *  mainwindow.h
 **********************************************************************
 * Copyright (C) 2020 MX Authors
 *
 * Authors: Adrian
 *          MX Linux <http://mxlinux.org>
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this package. If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>
#include <QFile>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPoint>
#include <QResizeEvent>
#include <QSettings>
#include <QToolTip>

#include "cmd.h"
#include "dockconfiguration.h"
#include "dockfilemanager.h"
#include "dockfileparser.h"
#include "dockiconmanager.h"
#include "icondragdrophandler.h"

namespace Ui
{
class MainWindow;
}

class MainWindow : public QDialog
{
    Q_OBJECT

public:
    enum Info { App, Command, Tooltip, Icon, Size, BgColor, BgHoverColor, BorderColor, BorderHoverColor, Extra };
    explicit MainWindow(QWidget *parent = nullptr, const QString &file = QString());
    ~MainWindow() override;

    bool checkDoneEditing();
    [[nodiscard]] bool isDockInMenu(const QString &file_name) const;

    void blockComboSignals(bool block);
    void deleteDock();
    void editDock(const QString &file_arg = QString());
    void moveDock();
    void moveIcon(int pos);
    void moveIconToPosition(int fromIndex, int toIndex);
    void newDock();
    void resetAdd();
    void setConnections();
    void setup(const QString &file = QString());
    void showApp(int i, int old_idx);
    void updateAppList(int idx);

    QString pickSlitLocation();
    [[nodiscard]] static QString inputDockName();

public slots:

private slots:
    void allItemsChanged();
    void buttonAbout_clicked();
    void buttonAdd_clicked();
    void buttonDelete_clicked();
    void buttonHelp_clicked();
    void buttonMoveLeft_clicked();
    void buttonMoveRight_clicked();
    void buttonNext_clicked();
    void buttonPrev_clicked();
    void buttonSave_clicked();
    void buttonSelectApp_clicked();
    void buttonSelectIcon_clicked();
    void checkApplyStyleToAll_stateChanged(int arg1);
    void comboBgColor_currentTextChanged();
    void comboBorderColor_currentTextChanged();
    void comboSize_currentTextChanged();
    void itemChanged();
    void lineEditCommand_textEdited();
    void lineEditTooltip_textEdited();
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void pickColor(QWidget *widget);
    void radioDesktop_toggled(bool checked);
    void setColor(QWidget *widget, const QColor &color);

private:
    void applyIconStyles(int selectedIndex);
    void updateUIFromConfiguration();
    void updateConfigurationFromUI();

    // Legacy methods (temporary during refactoring)
    void displayIcon(const QString &app_name, int location, const QString &custom_icon = QString());
    void createInsertionIndicators();
    void positionInsertionIndicators();
    void updateInsertionIndicators(const QPoint &mousePos);
    void parseFile(QFile &file);
    void addDockToMenu(const QString &file_name);
    [[nodiscard]] QPixmap findIcon(const QString &icon_name, const QSize &size);
    [[nodiscard]] static QString getDockName(const QString &file_name);

    Ui::MainWindow *ui;
    
    // New architecture components
    DockConfiguration *m_configuration;
    DockFileManager *m_fileManager;
    DockFileParser *m_fileParser;
    DockIconManager *m_iconManager;
    IconDragDropHandler *m_dragDropHandler;
    
    // Legacy support (temporary during refactoring)
    QSettings settings;
    bool changed = false;
    bool parsing = false;
    int index = 0;
    QList<QLabel *> list_icons;
    
    // Current file tracking
    QString currentFilePath;
    
    // Legacy variables (needed during migration)
    Cmd cmd;
    QString dock_name;
    QString file_name;
    QString file_content;
    
    // Drag and drop state
    QLabel *dragIndicator = nullptr;
    QList<QLabel *> insertionIndicators;
    bool dragging = false;
    int dragStartIndex = -1;
    QPoint dragStartPos;
    
    // Other legacy variables
    QString slit_location;
    
    // Temporary apps variable (will be removed after migration)
    QList<QStringList> apps;
    
    // Icon styling constants
    static constexpr int kIconBorderWidth = 4;
    static constexpr int kIconPadding = 4;
};

#endif
