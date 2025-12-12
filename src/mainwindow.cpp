/**********************************************************************
 *  mainwindow.cpp
 **********************************************************************
 * Copyright (C) 2020-2025 MX Authors
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
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QColorDialog>
#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>
#include <QProcess>
#include <QRegularExpression>
#include <QTimer>

#include "about.h"
#include "picklocation.h"

#ifndef VERSION
#define VERSION "?.?.?.?"
#endif

namespace
{
[[nodiscard]] QString quoteArgument(const QString &arg)
{
    if (arg.isEmpty()) {
        return QStringLiteral("''");
    }
    auto escaped = arg;
    escaped.replace(QLatin1Char('\''), QStringLiteral("'\"'\"'")); // POSIX-safe single-quote escaping
    return QLatin1Char('\'') + escaped + QLatin1Char('\'');
}

[[nodiscard]] QString quoteDouble(const QString &arg)
{
    auto escaped = arg;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return QLatin1Char('"') + escaped + QLatin1Char('"');
}

constexpr int kIconBorderWidth = 4;
constexpr int kIconPadding = 4; // Space reserved around the icon so borders stay outside the artwork

[[nodiscard]] QSize iconContainerSize(QSize iconSize)
{
    return iconSize
           + QSize(2 * (kIconBorderWidth + kIconPadding), 2 * (kIconBorderWidth + kIconPadding)); // padding + border
}
} // namespace

MainWindow::MainWindow(QWidget *parent, const QString &file)
    : QDialog(parent),
      ui(new Ui::MainWindow),
      m_configuration(new DockConfiguration(this)),
      m_fileManager(new DockFileManager(this)),
      m_fileParser(new DockFileParser(this)),
      m_iconManager(new DockIconManager(this)),
      m_dragDropHandler(new IconDragDropHandler(this))
{
    ui->setupUi(this);
    setConnections();
    setWindowFlags(Qt::Window); // for the close, min and max buttons

    // Connect signals from new architecture
    connect(m_configuration, &DockConfiguration::configurationModified, this, [this]() {
        if (parsing) {
            return;
        }
        changed = true;
        checkDoneEditing();
    });

    setup(file);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::renderIconAt(int location)
{
    if (location < 0 || location >= m_configuration->getApplicationCount()) {
        return;
    }

    while (listIcons.size() <= location) {
        auto *label = new QLabel(this);
        listIcons.append(label);
        ui->icons->addWidget(label);
    }

    DockIconInfo iconInfo = m_configuration->getApplication(location);
    m_iconManager->displayIcon(iconInfo, listIcons.at(location), index, location);

    // Ensure tooltips use the standard styling
    listIcons.at(location)->installEventFilter(this);
    syncDragHandler();
}

void MainWindow::renderIconsFromConfiguration()
{
    // Clear existing icons
    for (QLabel *icon : listIcons) {
        delete icon;
    }
    listIcons.clear();

    if (ui->icons->layout()) {
        QLayoutItem *item;
        while ((item = ui->icons->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
    }

    const int appCount = m_configuration->getApplicationCount();
    for (int i = 0; i < appCount; ++i) {
        renderIconAt(i);
    }

    syncDragHandler();
}

void MainWindow::syncDragHandler()
{
    if (m_dragDropHandler) {
        m_dragDropHandler->setIconLabels(listIcons);
    }
}

void MainWindow::applyIconStyles(int selectedIndex)
{
    if (listIcons.isEmpty() || m_configuration->isEmpty()) {
        return;
    }

    for (int i = 0; i < listIcons.size() && i < m_configuration->getApplicationCount(); ++i) {
        DockIconInfo iconInfo = m_configuration->getApplication(i);
        const bool isSelected = selectedIndex >= 0 && selectedIndex == i;
        m_iconManager->applyIconStyle(iconInfo, listIcons.at(i), isSelected);
    }
}

bool MainWindow::checkDoneEditing()
{
    const bool hasSelection = ui->buttonSelectApp->text() != tr("Select...") || !ui->lineEditCommand->text().isEmpty();

    // Save only becomes active after a user change
    ui->buttonSave->setEnabled(changed && hasSelection);

    const bool hasApps = !m_configuration->isEmpty();
    ui->buttonDelete->setEnabled(hasApps);

    ui->buttonAdd->setEnabled(true); // Always allow adding a new entry, even when empty

    if (hasSelection) {
        if (index != 0) {
            ui->buttonPrev->setEnabled(true);
        }
        if (index < m_configuration->getApplicationCount() - 1 && m_configuration->getApplicationCount() > 1) {
            ui->buttonNext->setEnabled(true);
        }
        return true;
    }

    ui->buttonSave->setEnabled(false);
    ui->buttonPrev->setEnabled(false);
    ui->buttonNext->setEnabled(false);
    return false;
}

// setup various items first time program runs
void MainWindow::setup(const QString &file)
{
    // Clean up any drag and drop state
    if (m_dragDropHandler) {
        m_dragDropHandler->cleanup();
    }

    m_configuration->clear();
    renderIconsFromConfiguration();
    index = 0;
    parsing = false;
    changed = false;

    changed = false;
    ui->buttonSave->setEnabled(false);
    this->setWindowTitle(QStringLiteral("MX Dockmaker"));
    ui->labelUsage->setText(tr("1. Add applications to the dock one at a time\n"
                               "2. Select a .desktop file or enter a command for the application you want\n"
                               "3. Select icon attributes for size, background (black is standard) and border\n"
                               "4. Press \"Add application\" to continue or \"Save\" to finish"));
    this->adjustSize();

    blockComboSignals(true);

    ui->comboSize->setCurrentIndex(ui->comboSize->findText(QStringLiteral("48x48")));

    // Write configs if not there
    settings.setValue(QStringLiteral("BackgroundColor"),
                      settings.value(QStringLiteral("BackgroundColor"), "black").toString());
    settings.setValue(QStringLiteral("BackgroundHoverColor"),
                      settings.value(QStringLiteral("BackgroundHoverColor"), "black").toString());
    settings.setValue(QStringLiteral("FrameColor"), settings.value(QStringLiteral("FrameColor"), "white").toString());
    settings.setValue(QStringLiteral("FrameHoverColor"),
                      settings.value(QStringLiteral("FrameHoverColor"), "white").toString());
    settings.setValue(QStringLiteral("Size"), settings.value(QStringLiteral("Size"), "48x48").toString());
    // Set default values
    setColor(ui->widgetBackground, settings.value("BackgroundColor").toString());
    setColor(ui->widgetHoverBackground, settings.value("BackgroundHoverColor").toString());
    setColor(ui->widgetBorder, settings.value("FrameColor").toString());
    setColor(ui->widgetHoverBorder, settings.value("FrameHoverColor").toString());

    blockComboSignals(false);
    ui->buttonSave->setEnabled(false);

    if (!file.isEmpty() && QFile::exists(file)) {
        editDock(file);
        return;
    }

    auto *mbox = new QMessageBox(this);
    mbox->setText(tr("This tool allows you to create a new dock with one or more applications. "
                     "You can also edit or delete a dock created earlier."));
    mbox->setIcon(QMessageBox::Question);
    mbox->setWindowTitle(tr("Operation mode"));

    auto *moveBtn = mbox->addButton(tr("&Move"), QMessageBox::NoRole);
    auto *deleteBtn = mbox->addButton(tr("&Delete"), QMessageBox::NoRole);
    auto *editBtn = mbox->addButton(tr("&Edit"), QMessageBox::NoRole);
    auto *createBtn = mbox->addButton(tr("C&reate"), QMessageBox::NoRole);
    auto *closeBtn = mbox->addButton(tr("&Close"), QMessageBox::RejectRole);

    this->hide();
    mbox->exec();

    auto *clickedButton = mbox->clickedButton();

    if (clickedButton == closeBtn) {
        QTimer::singleShot(0, QApplication::instance(), &QGuiApplication::quit);
    } else if (clickedButton == moveBtn) {
        moveDock();
    } else if (clickedButton == deleteBtn) {
        this->show();
        deleteDock();
        setup();
    } else if (clickedButton == editBtn) {
        editDock();
    } else if (clickedButton == createBtn) {
        newDock();
    }

    delete mbox;
}

[[nodiscard]] QString MainWindow::inputDockName()
{
    bool ok = false;
    QString text = QInputDialog::getText(nullptr, tr("Dock name"), tr("Enter the name to show in the Menu:"),
                                         QLineEdit::Normal, QString(), &ok);
    if (ok && !text.isEmpty()) {
        return text;
    }
    return {};
}

void MainWindow::allItemsChanged()
{
    changed = true;
    for (int i = 0; i < m_configuration->getApplicationCount(); ++i) {
        DockIconInfo iconInfo = m_configuration->getApplication(i);
        // Update iconInfo with current UI settings
        iconInfo.size = ui->comboSize->currentText();
        iconInfo.backgroundColor = ui->widgetBackground->palette().color(QWidget::backgroundRole());
        iconInfo.hoverBackground = ui->widgetHoverBackground->palette().color(QWidget::backgroundRole());
        iconInfo.borderColor = ui->widgetBorder->palette().color(QWidget::backgroundRole());
        iconInfo.hoverBorder = ui->widgetHoverBorder->palette().color(QWidget::backgroundRole());
        m_configuration->updateApplication(i, iconInfo);
        const quint8 width = ui->comboSize->currentText().section(QStringLiteral("x"), 0, 0).toUShort();
        const QSize size(width, width);
        const QSize containerSize = iconContainerSize(size);
        listIcons.at(i)->setPixmap(listIcons.at(i)->pixmap(Qt::ReturnByValue).scaled(size));
        listIcons.at(i)->setAlignment(Qt::AlignCenter);
        listIcons.at(i)->setFixedSize(containerSize);
    }
    applyIconStyles(index);
    checkDoneEditing();
}

QString MainWindow::pickSlitLocation()
{
    auto *const pick = new PickLocation(slitLocation, this);
    pick->exec();
    return pick->button;
}

void MainWindow::itemChanged()
{
    changed = true;
    updateAppList(index);
    checkDoneEditing();
    renderIconAt(index);

    applyIconStyles(index);

    if (ui->checkApplyStyleToAll->isChecked()) {
        allItemsChanged();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    auto *clickedWidget = this->childAt(event->pos());

    if (!clickedWidget) {
        return;
    }

    if (clickedWidget == ui->widgetBackground || clickedWidget == ui->widgetHoverBackground
        || clickedWidget == ui->widgetBorder || clickedWidget == ui->widgetHoverBorder) {
        pickColor(clickedWidget);
        return;
    }

    if (!checkDoneEditing()) {
        return;
    }

    if (event->button() == Qt::LeftButton && m_dragDropHandler) {
        const int clickedIndex = m_dragDropHandler->handleMousePress(event, clickedWidget, listIcons);
        if (clickedIndex >= 0) {
            index = clickedIndex;
            showApp(index);
            return;
        }
    }

    QDialog::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragDropHandler && m_dragDropHandler->handleMouseMove(event, this)) {
        setCursor(Qt::ClosedHandCursor);
    }
    QDialog::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragDropHandler) {
        const int startIndex = m_dragDropHandler->getDragStartIndex();
        const int targetIndex = m_dragDropHandler->handleMouseRelease(event);
        if (targetIndex >= 0 && startIndex >= 0) {
            moveIconToPosition(startIndex, targetIndex);
        }
    }

    setCursor(Qt::ArrowCursor); // Reset cursor

    applyIconStyles(index);

    QDialog::mouseReleaseEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);

    if (m_dragDropHandler) {
        m_dragDropHandler->handleResizeEvent(event);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // Handle tooltips for icon labels with standard Qt styling
    if (event->type() == QEvent::ToolTip) {
        auto *label = qobject_cast<QLabel *>(obj);
        if (label && listIcons.contains(label)) {
            QString tooltip = label->property("icon_tooltip").toString();
            if (!tooltip.isEmpty()) {
                QToolTip::showText(QCursor::pos(), tooltip);
                return true; // Event handled
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_dragDropHandler) {
        m_dragDropHandler->cleanup();
    }

    m_configuration->clear();
    renderIconsFromConfiguration();

    event->accept();
    this->hide();
    QTimer::singleShot(0, this, [this]() { setup(""); }); // Show operation selection dialog again
}

void MainWindow::updateAppList(int idx)
{
    // Create DockIconInfo from current UI state
    DockIconInfo iconInfo;
    iconInfo.appName = ui->buttonSelectApp->text();
    iconInfo.command = ui->lineEditCommand->text();
    iconInfo.tooltip = ui->lineEditTooltip->text();
    iconInfo.customIcon = ui->buttonSelectIcon->text();
    iconInfo.size = ui->comboSize->currentText();
    iconInfo.backgroundColor = ui->widgetBackground->palette().color(QWidget::backgroundRole());
    iconInfo.hoverBackground = ui->widgetHoverBackground->palette().color(QWidget::backgroundRole());
    iconInfo.borderColor = ui->widgetBorder->palette().color(QWidget::backgroundRole());
    iconInfo.hoverBorder = ui->widgetHoverBorder->palette().color(QWidget::backgroundRole());
    iconInfo.extraOptions = ui->buttonSelectApp->property("extra_options").toString();
    if (iconInfo.isDesktopFile() || iconInfo.customIcon == tr("Select icon...")) {
        iconInfo.customIcon.clear();
    }
    if (!iconInfo.isDesktopFile() && iconInfo.customIcon.isEmpty()) {
        iconInfo.customIcon = QStringLiteral("application-x-executable");
    }

    // Update configuration
    if (idx < m_configuration->getApplicationCount()) {
        m_configuration->updateApplication(idx, iconInfo);
    } else {
        m_configuration->addApplication(iconInfo, false);
    }
}

void MainWindow::deleteDock()
{
    hide();

    const QString selectedDock
        = QFileDialog::getOpenFileName(this, tr("Select dock to delete"), QDir::homePath() + "/.fluxbox/scripts",
                                       tr("Dock Files (*.mxdk);;All Files (*.*)"));

    if (!selectedDock.isEmpty()) {
        const QMessageBox::StandardButton confirmation = QMessageBox::question(
            this, tr("Confirmation"), tr("Are you sure you want to delete %1?").arg(selectedDock),
            QMessageBox::Yes | QMessageBox::Cancel);

        if (confirmation == QMessageBox::Yes) {
            if (!m_fileManager->deleteDockFile(selectedDock, true)) {
                QMessageBox::warning(this, tr("Error"),
                                     tr("Failed to delete the selected dock: %1").arg(m_fileManager->getLastError()));
            }
        }
    }

    show();
}

// block/unblock all relevant signals when loading stuff into GUI
void MainWindow::blockComboSignals(bool block)
{
    ui->comboSize->blockSignals(block);
}

void MainWindow::moveDock()
{
    this->hide();

    const QString selected_dock
        = QFileDialog::getOpenFileName(this, tr("Select dock to move"), QDir::homePath() + "/.fluxbox/scripts",
                                       tr("Dock Files (*.mxdk);;All Files (*.*)"));
    if (selected_dock.isEmpty()) {
        setup();
        return;
    }

    slitLocation = pickSlitLocation();
    if (!m_fileManager->moveDockFile(selected_dock, slitLocation)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to move dock: %1").arg(m_fileManager->getLastError()));
    }
    setup();
    this->show();
}

// Move icon: pos -1 to shift left, +1 to shift right
void MainWindow::moveIcon(int pos)
{
    int newIndex = index + pos;
    if (newIndex < 0 || newIndex >= m_configuration->getApplicationCount()) {
        return;
    }

    changed = true;
    // Move application in the configuration
    m_configuration->moveApplication(index, newIndex);

    auto refreshIconAt = [this](int pos) { renderIconAt(pos); };

    refreshIconAt(index);
    refreshIconAt(newIndex);

    // Update the current index
    index = newIndex;

    applyIconStyles(index);

    // Refresh the displayed application
    showApp(index);

    checkDoneEditing();
}

// Move icon from one position to another
void MainWindow::moveIconToPosition(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || fromIndex >= m_configuration->getApplicationCount() || toIndex < 0
        || toIndex >= m_configuration->getApplicationCount() || fromIndex == toIndex) {
        return;
    }

    const int step = (toIndex > fromIndex) ? 1 : -1;

    // Align selection with the dragged icon so moveIcon updates focus consistently
    index = fromIndex;
    while (index != toIndex) {
        moveIcon(step);
    }
}

void MainWindow::buttonSave_clicked()
{
    slitLocation = pickSlitLocation();
    m_configuration->setSlitLocation(slitLocation);

    QString targetFile = m_configuration->getFileName();
    bool newFile = false;

    if (!targetFile.isEmpty() && QFileInfo::exists(targetFile)) {
        const auto reply = QMessageBox::question(this, tr("Overwrite?"), tr("Do you want to overwrite the dock file?"),
                                                 QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (reply == QMessageBox::Cancel) {
            return;
        }
        if (reply == QMessageBox::No) {
            targetFile.clear();
        }
    }

    if (targetFile.isEmpty()) {
        targetFile = QFileDialog::getSaveFileName(this, tr("Save file"), QDir::homePath() + "/.fluxbox/scripts",
                                                  tr("Dock Files (*.mxdk);;All Files (*.*)"));
        if (targetFile.isEmpty()) {
            return;
        }
        if (!targetFile.endsWith(QLatin1String(".mxdk"))) {
            targetFile += QLatin1String(".mxdk");
        }
        newFile = true;
    }

    m_configuration->setFileName(targetFile);

    QString dockName = m_configuration->getDockName();
    if (dockName.isEmpty() || newFile) {
        dockName = inputDockName();
    }
    if (dockName.isEmpty()) {
        dockName = QFileInfo(targetFile).baseName();
    }
    m_configuration->setDockName(dockName);

    if (!m_fileManager->saveConfiguration(*m_configuration, targetFile, true)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to save the dock: %1").arg(m_fileManager->getLastError()));
        return;
    }

    if (!m_fileManager->isInMenu(targetFile)) {
        m_fileManager->addToMenu(targetFile, dockName);
    }

    m_configuration->markAsSaved();
    changed = false;

    QMessageBox::information(this, tr("Dock saved"),
                             tr("The dock has been saved.\n\n"
                                "To edit the newly created dock please select 'Edit an existing dock'."));
    QProcess::execute(QStringLiteral("pkill"), {"wmalauncher"});
    QProcess::startDetached(targetFile, {});
    index = 0;
    resetAdd();
    ui->buttonSave->setEnabled(false);
    ui->buttonDelete->setEnabled(false);
    setup();
}

// About button clicked
void MainWindow::buttonAbout_clicked()
{
    this->hide();
    displayAboutMsgBox(
        tr("About %1").arg(tr("MX Dockmaker")),
        R"(<p align="center"><b><h2>MX Dockmaker</h2></b></p><p align="center">)" + tr("Version: ") + VERSION
            + "</p><p align=\"center\"><h3>" + tr("Description goes here")
            + "</h3></p><p align=\"center\"><a href=\"http://mxlinux.org\">http://mxlinux.org</a><br /></p>"
              "<p align=\"center\">"
            + tr("Copyright (c) MX Linux") + "<br /><br /></p>",
        QStringLiteral("/usr/share/doc/mx-dockmaker/license.html"), tr("%1 License").arg(this->windowTitle()));

    this->show();
}

// Help button clicked
void MainWindow::buttonHelp_clicked()
{
    const QString url = QStringLiteral("https://mxlinux.org/wiki/help-files/help-mx-dockmaker/");
    displayDoc(url, tr("%1 Help").arg(this->windowTitle()));
}

void MainWindow::comboSize_currentTextChanged()
{
    itemChanged();
}

void MainWindow::comboBgColor_currentTextChanged()
{
    itemChanged();
}

void MainWindow::comboBorderColor_currentTextChanged()
{
    itemChanged();
}

void MainWindow::buttonNext_clicked()
{
    blockComboSignals(true);
    updateAppList(index);
    index++;
    showApp(index);
    blockComboSignals(false);
}

void MainWindow::buttonDelete_clicked()
{
    if (m_configuration->isEmpty()) {
        ui->buttonDelete->setEnabled(false);
        return;
    }

    blockComboSignals(true);
    if (index >= 0 && index < m_configuration->getApplicationCount()) {
        m_configuration->removeApplication(index);
    }
    blockComboSignals(false);

    // Rebuild icon labels to keep indices aligned and avoid stale pointers
    renderIconsFromConfiguration();

    const int count = m_configuration->getApplicationCount();
    if (count == 0) {
        index = 0;
        ui->buttonDelete->setEnabled(false);
        resetAdd();
        syncDragHandler();
        checkDoneEditing();
        return;
    }

    index = std::min(index, count - 1);
    changed = true;
    ui->buttonSave->setEnabled(true);
    ui->buttonDelete->setEnabled(true);
    showApp(index);

    syncDragHandler();
    checkDoneEditing();
}

void MainWindow::resetAdd()
{
    ui->buttonSelectApp->setText(tr("Select..."));
    ui->buttonSelectApp->setProperty("extra_options", QString());
    ui->radioDesktop->click();
    emit ui->radioDesktop->toggled(true);
    ui->buttonAdd->setEnabled(true);
    ui->lineEditTooltip->clear();

    QString size;
    if (ui->checkApplyStyleToAll->isChecked() && index != 0
        && m_configuration->getApplicationCount() > 0) { // set style according to the first item
        DockIconInfo firstIcon = m_configuration->getApplication(0);
        size = firstIcon.size;
    } else {
        size = settings.value(QStringLiteral("Size"), "48x48").toString();
    }

    blockComboSignals(true);
    ui->comboSize->setCurrentIndex(ui->comboSize->findText(size));
    blockComboSignals(false);

    ui->buttonSelectIcon->setToolTip(QString());
    ui->buttonSelectIcon->setStyleSheet(QStringLiteral("text-align: center; padding: 3px;"));
}

void MainWindow::setConnections()
{
    connect(ui->buttonAbout, &QPushButton::clicked, this, &MainWindow::buttonAbout_clicked);
    connect(ui->buttonAdd, &QPushButton::clicked, this, &MainWindow::buttonAdd_clicked);
    connect(ui->buttonDelete, &QPushButton::clicked, this, &MainWindow::buttonDelete_clicked);
    connect(ui->buttonHelp, &QPushButton::clicked, this, &MainWindow::buttonHelp_clicked);
    connect(ui->buttonMoveLeft, &QPushButton::clicked, this, &MainWindow::buttonMoveLeft_clicked);
    connect(ui->buttonMoveRight, &QPushButton::clicked, this, &MainWindow::buttonMoveRight_clicked);
    connect(ui->buttonNext, &QPushButton::clicked, this, &MainWindow::buttonNext_clicked);
    connect(ui->buttonPrev, &QPushButton::clicked, this, &MainWindow::buttonPrev_clicked);
    connect(ui->buttonSave, &QPushButton::clicked, this, &MainWindow::buttonSave_clicked);
    connect(ui->buttonSelectApp, &QPushButton::clicked, this, &MainWindow::buttonSelectApp_clicked);
    connect(ui->buttonSelectIcon, &QPushButton::clicked, this, &MainWindow::buttonSelectIcon_clicked);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(ui->checkApplyStyleToAll, &QCheckBox::checkStateChanged, this,
            &MainWindow::checkApplyStyleToAll_stateChanged);
#else
    connect(ui->checkApplyStyleToAll, &QCheckBox::stateChanged, this, &MainWindow::checkApplyStyleToAll_stateChanged);
#endif
    connect(ui->comboSize, &QComboBox::currentTextChanged, this, &MainWindow::comboSize_currentTextChanged);
    connect(ui->lineEditCommand, &QLineEdit::textEdited, this, &MainWindow::lineEditCommand_textEdited);
    connect(ui->lineEditTooltip, &QLineEdit::textEdited, this, &MainWindow::lineEditTooltip_textEdited);
    connect(ui->radioDesktop, &QRadioButton::toggled, this, &MainWindow::radioDesktop_toggled);
    connect(ui->toolBackground, &QToolButton::clicked, this, [this] { pickColor(ui->widgetBackground); });
    connect(ui->toolBorder, &QToolButton::clicked, this, [this] { pickColor(ui->widgetBorder); });
    connect(ui->toolHoverBackground, &QToolButton::clicked, this, [this] { pickColor(ui->widgetHoverBackground); });
    connect(ui->toolHoverBorder, &QToolButton::clicked, this, [this] { pickColor(ui->widgetHoverBorder); });
}

void MainWindow::showApp(int idx)
{
    const int count = m_configuration->getApplicationCount();
    if (idx < 0 || idx >= count) {
        return;
    }

    ui->radioCommand->blockSignals(true);
    ui->radioDesktop->blockSignals(true);

    const DockIconInfo iconInfo = m_configuration->getApplication(idx);
    ui->buttonSelectApp->setText(iconInfo.appName);
    if (iconInfo.isDesktopFile() || iconInfo.command.isEmpty()) {
        ui->radioDesktop->setChecked(true);
        ui->lineEditCommand->clear();
        ui->buttonSelectApp->setEnabled(true);
        ui->lineEditCommand->setEnabled(false);
        ui->buttonSelectIcon->setEnabled(false);
        ui->buttonSelectIcon->setText(tr("Select icon..."));
        ui->buttonSelectIcon->setStyleSheet(QStringLiteral("text-align: center; padding: 3px;"));
    } else {
        ui->radioCommand->setChecked(true);
        ui->buttonSelectApp->setText(tr("Select..."));
        ui->buttonSelectApp->setEnabled(false);
        ui->lineEditCommand->setEnabled(true);
        ui->buttonSelectIcon->setEnabled(true);
        ui->lineEditCommand->setText(iconInfo.command);
        ui->buttonSelectIcon->setText(iconInfo.customIcon);
        ui->buttonSelectIcon->setToolTip(iconInfo.customIcon);
        ui->buttonSelectIcon->setStyleSheet(QStringLiteral("text-align: right; padding: 3px;"));
    }

    ui->lineEditTooltip->setText(iconInfo.tooltip);

    ui->radioCommand->blockSignals(false);
    ui->radioDesktop->blockSignals(false);

    applyIconStyles(idx);

    blockComboSignals(true);
    ui->comboSize->setCurrentIndex(ui->comboSize->findText(iconInfo.size));
    setColor(ui->widgetBackground, iconInfo.backgroundColor.name());
    setColor(ui->widgetBorder, iconInfo.borderColor.name());
    setColor(ui->widgetHoverBackground, iconInfo.hoverBackground.name());
    setColor(ui->widgetHoverBorder, iconInfo.hoverBorder.name());
    ui->buttonSelectApp->setProperty("extra_options", iconInfo.extraOptions);
    blockComboSignals(false);

    // set buttons
    ui->buttonNext->setDisabled(idx == m_configuration->getApplicationCount() - 1
                                || m_configuration->getApplicationCount() == 1);
    ui->buttonPrev->setDisabled(idx == 0);
    ui->buttonAdd->setDisabled(m_configuration->isEmpty());
    ui->buttonDelete->setEnabled(true);
    ui->buttonMoveLeft->setDisabled(idx == 0);
    ui->buttonMoveRight->setDisabled(idx == m_configuration->getApplicationCount() - 1);
    checkDoneEditing(); // Update button states based on current UI
}

void MainWindow::buttonSelectApp_clicked()
{
    const QString selected = QFileDialog::getOpenFileName(
        this, tr("Select .desktop file"), QStringLiteral("/usr/share/applications"), tr("Desktop Files (*.desktop)"));
    const QString file = QFileInfo(selected).fileName();
    if (!file.isEmpty()) {
        // Store the selected desktop file name in the current icon info
        int currentIdx = index;
        if (currentIdx >= 0 && currentIdx < m_configuration->getApplicationCount()) {
            DockIconInfo iconInfo = m_configuration->getApplication(currentIdx);
            iconInfo.appName = file;
            m_configuration->updateApplication(currentIdx, iconInfo);
        }
        ui->buttonSelectApp->setText(file);
        ui->buttonAdd->setEnabled(true);
        ui->buttonSelectApp->setProperty("extra_options", QString()); // reset extra options when changing the app.
        ui->lineEditTooltip->clear();
        itemChanged();
    }
}

void MainWindow::editDock(const QString &file_arg)
{
    m_configuration->clear();
    renderIconsFromConfiguration();
    index = 0;

    QString selected_dock;
    if (!file_arg.isEmpty() && QFile::exists(file_arg)) {
        selected_dock = file_arg;
    } else {
        selected_dock
            = QFileDialog::getOpenFileName(this, tr("Select a dock file"), QDir::homePath() + "/.fluxbox/scripts",
                                           tr("Dock Files (*.mxdk);;All Files (*.*)"));
    }

    if (!QFileInfo::exists(selected_dock)) {
        QMessageBox::warning(this, tr("No file selected"),
                             tr("You haven't selected any dock file to edit.\nCreating a new dock instead."));
        return;
    }
    parsing = true;

    if (!m_fileManager->loadConfiguration(selected_dock, *m_configuration)) {
        qDebug() << "Could not load configuration:" << selected_dock << m_fileManager->getLastError();
        QMessageBox::warning(this, tr("Could not open file"),
                             tr("Could not open selected file.\nCreating a new dock instead."));
        parsing = false;
        return;
    }

    m_configuration->setFileName(selected_dock);

    const QString dockName = m_fileParser->extractDockName(selected_dock);
    if (!dockName.isEmpty()) {
        m_configuration->setDockName(dockName);
    }

    slitLocation = m_configuration->getSlitLocation();

    renderIconsFromConfiguration();
    if (!m_configuration->isEmpty()) {
        index = 0;
        showApp(index);
        ui->buttonDelete->setEnabled(true);
    } else {
        resetAdd();
        ui->buttonDelete->setEnabled(false);
    }

    ui->labelUsage->setText(tr("1. Edit applications one at a time using the Back and Next buttons\n"
                               "2. Add or delete applications as you like\n"
                               "3. When finished click Save"));
    parsing = false;
    changed = false;
    checkDoneEditing();
    this->show();
}

void MainWindow::newDock()
{
    this->show();
    m_configuration->clear();
    renderIconsFromConfiguration();
    index = 0;

    resetAdd();
    checkDoneEditing(); // Re-evaluate button states after reset
    ui->buttonSave->setEnabled(false);
    ui->buttonDelete->setEnabled(false);
}

void MainWindow::buttonPrev_clicked()
{
    blockComboSignals(true);
    updateAppList(index);
    int oldIndex = index;
    showApp(--index);
    blockComboSignals(false);
}

void MainWindow::radioDesktop_toggled(bool checked)
{
    ui->lineEditCommand->clear();
    ui->buttonSelectApp->setEnabled(checked);
    ui->lineEditCommand->setEnabled(!checked);
    ui->buttonSelectIcon->setEnabled(!checked);
    if (checked) {
        ui->buttonSelectIcon->setText(tr("Select icon..."));
        ui->buttonSelectIcon->setStyleSheet(QStringLiteral("text-align: center; padding: 3px;"));
    } else {
        ui->buttonSelectApp->setText(tr("Select..."));
    }
    if (!parsing) {
        checkDoneEditing();
    }
}

void MainWindow::pickColor(QWidget *widget)
{
    QColor color = QColorDialog::getColor(widget->palette().color(QWidget::backgroundRole()));
    if (color.isValid()) {
        setColor(widget, color);
        itemChanged();
        if (widget == ui->widgetBackground) {
            setColor(ui->widgetHoverBackground, "black");
        } else if (widget == ui->widgetBorder) {
            setColor(ui->widgetHoverBorder, "white");
        }
    }
}

void MainWindow::setColor(QWidget *widget, const QColor &color)
{
    if (color.isValid()) {
        QPalette pal = palette();
        pal.setColor(QPalette::Window, color);
        widget->setAutoFillBackground(true);
        widget->setPalette(pal);
    }
}

void MainWindow::buttonSelectIcon_clicked()
{
    ui->buttonSave->setDisabled(ui->buttonNext->isEnabled());
    QString default_folder;
    if (ui->buttonSelectIcon->text() != tr("Select icon...") && QFileInfo::exists(ui->buttonSelectIcon->text())) {
        QFileInfo f_info(ui->buttonSelectIcon->text());
        default_folder = f_info.canonicalPath();
    } else {
        default_folder = QStringLiteral("/usr/share/icons/");
    }
    QString selected = QFileDialog::getOpenFileName(this, tr("Select icon"), default_folder,
                                                    tr("Icons (*.png *.jpg *.bmp *.xpm *.svg)"));
    QString file = QFileInfo(selected).fileName();
    if (!file.isEmpty()) {
        ui->buttonSelectIcon->setText(selected);
        ui->buttonSelectIcon->setToolTip(selected);
        ui->buttonSelectIcon->setStyleSheet(QStringLiteral("text-align: right; padding: 3px;"));
        updateAppList(index);
        renderIconAt(index);
        applyIconStyles(index);
        changed = true;
    }
    if (!parsing) {
        checkDoneEditing();
    }
}

void MainWindow::lineEditCommand_textEdited()
{
    changed = true;
    ui->buttonNext->setEnabled(ui->buttonSelectIcon->text() != tr("Select icon..."));
    updateAppList(index);
    if (!parsing) {
        checkDoneEditing();
    }
}

void MainWindow::lineEditTooltip_textEdited()
{
    // Sanitize input by removing quotes
    QString sanitizedText = ui->lineEditTooltip->text();
    sanitizedText.remove(QLatin1Char('\''));
    sanitizedText.remove(QLatin1Char('"'));
    if (sanitizedText != ui->lineEditTooltip->text()) {
        ui->lineEditTooltip->setText(sanitizedText);
    }

    changed = true;
    updateAppList(index);
    if (!parsing) {
        checkDoneEditing();
    }
}

void MainWindow::buttonAdd_clicked()
{
    // Calculate insertion position: after current item (or at 0 if empty)
    int insertPos = m_configuration->isEmpty() ? 0 : index + 1;

    // Create a new empty DockIconInfo with default settings
    DockIconInfo iconInfo;
    iconInfo.appName.clear();
    iconInfo.command.clear();
    iconInfo.tooltip.clear();
    iconInfo.customIcon.clear();
    iconInfo.extraOptions.clear();

    // Use default size and colors from settings or first item if "apply style to all" is checked
    if (ui->checkApplyStyleToAll->isChecked() && !m_configuration->isEmpty()) {
        DockIconInfo firstIcon = m_configuration->getApplication(0);
        iconInfo.size = firstIcon.size;
        iconInfo.backgroundColor = firstIcon.backgroundColor;
        iconInfo.hoverBackground = firstIcon.hoverBackground;
        iconInfo.borderColor = firstIcon.borderColor;
        iconInfo.hoverBorder = firstIcon.hoverBorder;
    } else {
        iconInfo.size = settings.value(QStringLiteral("Size"), "48x48").toString();
        iconInfo.backgroundColor = QColor(settings.value(QStringLiteral("BackgroundColor"), "black").toString());
        iconInfo.hoverBackground = QColor(settings.value(QStringLiteral("BackgroundHoverColor"), "black").toString());
        iconInfo.borderColor = QColor(settings.value(QStringLiteral("FrameColor"), "white").toString());
        iconInfo.hoverBorder = QColor(settings.value(QStringLiteral("FrameHoverColor"), "white").toString());
    }

    // Insert at the calculated position
    m_configuration->insertApplication(insertPos, iconInfo, false);

    // Update index to point to the newly inserted item
    index = insertPos;

    resetAdd();
    renderIconsFromConfiguration();
    showApp(index);

    // Ensure the newly added icon has focus/selection
    applyIconStyles(index);
}

void MainWindow::buttonMoveLeft_clicked()
{
    if (index == 0) {
        return;
    }
    moveIcon(-1);
}

void MainWindow::buttonMoveRight_clicked()
{
    if (index == m_configuration->getApplicationCount() - 1) {
        return;
    }
    moveIcon(1);
}

void MainWindow::checkApplyStyleToAll_stateChanged(int arg1)
{
    if (arg1 == Qt::Checked) {
        allItemsChanged();
    }
}
