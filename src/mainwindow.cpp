/**********************************************************************
 *  mainwindow.cpp
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
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QColorDialog>
#include <QDebug>
#include <QFileDialog>
#include <QRegularExpression>
#include <QInputDialog>
#include <QProcess>
#include <QSet>
#include <QTimer>

#include "picklocation.h"
#include "about.h"

#ifndef VERSION
    #define VERSION "?.?.?.?"
#endif
#include "cmd.h"
#include <sys/stat.h>

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
} // namespace

MainWindow::MainWindow(QWidget *parent, const QString &file)
    : QDialog(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setConnections();
    setWindowFlags(Qt::Window); // for the close, min and max buttons
    setup(file);
}

MainWindow::~MainWindow()
{
    cmd.close();
    delete ui;
}

bool MainWindow::isDockInMenu(const QString &file_name) const
{
    if (dock_name.isEmpty()) {
        return false;
    }
    return getDockName(file_name) == dock_name;
}

void MainWindow::displayIcon(const QString &app_name, int location)
{
    QString icon;
    if (ui->buttonSelectApp->text().endsWith(QLatin1String(".desktop"))) {
        QFile file("/usr/share/applications/" + app_name);
        if (file.open(QFile::ReadOnly)) {
            QString text = file.readAll();
            file.close();
            QRegularExpression re(QStringLiteral("^Icon=(.*)$"), QRegularExpression::MultilineOption);
            auto match = re.match(text);
            if (match.hasMatch()) {
                icon = match.captured(1);
            } else {
                qDebug() << "Could not find icon in file " << file.fileName();
            }
        } else {
            qDebug() << "Could not open file " << file.fileName();
        }
    } else {
        icon = ui->buttonSelectIcon->text();
    }
    const quint8 width = ui->comboSize->currentText().section(QStringLiteral("x"), 0, 0).toUShort();
    const QSize size(width, width);
    const QPixmap pix = findIcon(icon, size).scaled(size);
    if (location == list_icons.size()) {
        list_icons << new QLabel(this);
        ui->icons->addWidget(list_icons.constLast());
    }
    list_icons.at(location)->setPixmap(pix);
    list_icons.at(location)->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    list_icons.at(location)->setStyleSheet(
        "background-color: " + ui->widgetBackground->palette().color(QWidget::backgroundRole()).name()
        + ";border: 4px solid " + ui->widgetBorder->palette().color(QWidget::backgroundRole()).name() + ";");
}

bool MainWindow::checkDoneEditing()
{
    if (!apps.isEmpty()) {
        ui->buttonDelete->setEnabled(true);
    }

    if (ui->buttonSelectApp->text() != tr("Select...") || !ui->lineEditCommand->text().isEmpty()) {
        if (changed) {
            ui->buttonSave->setEnabled(true);
        }
        ui->buttonAdd->setEnabled(true);
        if (index != 0) {
            ui->buttonPrev->setEnabled(true);
        }
        if (index < apps.size() - 1 && apps.size() > 1) {
            ui->buttonNext->setEnabled(true);
        }
        return true;
    } else {
        ui->buttonSave->setEnabled(false);
        ui->buttonPrev->setEnabled(false);
        ui->buttonNext->setEnabled(false);
        ui->buttonAdd->setEnabled(false);
        return false;
    }
}

// setup versious items first time program runs
void MainWindow::setup(const QString &file)
{
    changed = false;
    this->setWindowTitle(QStringLiteral("MX Dockmaker"));
    ui->labelUsage->setText(tr("1. Add applications to the dock one at a time\n"
                               "2. Select a .desktop file or enter a command for the application you want\n"
                               "3. Select icon attributes for size, background (black is standard) and border\n"
                               "4. Press \"Add application\" to continue or \"Save\" to finish"));
    this->adjustSize();

    blockComboSignals(true);

    while (ui->icons->layout()->count() > 0) {
        delete ui->icons->layout()->itemAt(0)->widget();
    }

    ui->comboSize->setCurrentIndex(ui->comboSize->findText(QStringLiteral("48x48")));
    file_content.clear();

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

    if (!file.isEmpty() && QFile::exists(file)) {
        editDock(file);
        return;
    }

    auto *mbox = new QMessageBox(nullptr);
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
        this->show();
        editDock();
    } else if (clickedButton == createBtn) {
        newDock();
    }

    delete mbox;
}

// Find icon file by name
[[nodiscard]] QPixmap MainWindow::findIcon(QString icon_name, QSize size)
{
    if (icon_name.isEmpty()) {
        return {};
    }
    if (QFileInfo::exists("/" + icon_name)) {
        return QIcon(icon_name).pixmap(size);
    }

    QString search_term = icon_name;
    if (!icon_name.endsWith(QLatin1String(".png")) && !icon_name.endsWith(QLatin1String(".svg"))
        && !icon_name.endsWith(QLatin1String(".xpm"))) {
        search_term = icon_name + ".*";
    }

    icon_name.remove(QRegularExpression(QStringLiteral(R"(\.png$|\.svg$|\.xpm$")")));

    // return the icon from the theme if it exists
    if (!QIcon::fromTheme(icon_name).isNull()) {
        return QIcon::fromTheme(icon_name).pixmap(size);
    }

    // Try to find in most obvious places
    QStringList search_paths {QDir::homePath() + "/.local/share/icons/", "/usr/share/pixmaps/",
                              "/usr/local/share/icons/", "/usr/share/icons/hicolor/48x48/apps/"};
    for (const QString &path : search_paths) {
        if (!QFileInfo::exists(path)) {
            search_paths.removeOne(path);
            continue;
        }
        for (const QString ext : {".png", ".svg", ".xpm"}) {
            QString file = path + icon_name + ext;
            if (QFileInfo::exists(file)) {
                return QIcon(file).pixmap(QSize());
            }
        }
    }

    // Search recursive
    search_paths.append(QStringLiteral("/usr/share/icons/hicolor/48x48/"));
    search_paths.append(QStringLiteral("/usr/share/icons/hicolor/"));
    search_paths.append(QStringLiteral("/usr/share/icons/"));
    const QString out = cmd.getCmdOut("find " + search_paths.join(QStringLiteral(" ")) + " -iname \"" + search_term
                                          + "\" -print -quit 2>/dev/null",
                                      true);
    return (!out.isEmpty()) ? QIcon(out).pixmap(size) : QPixmap();
}

[[nodiscard]] QString MainWindow::getDockName(const QString &file_name)
{
    const QRegularExpression re_file(".*" + QFileInfo(file_name).fileName());
    const QRegularExpression re_name(QStringLiteral("\\(.*\\)"));

    // check if dock name is in /usr/share...
    QFile file(QStringLiteral("/usr/share/mxflux/menu/appearance"));

    if (file.open(QFile::Text | QFile::ReadOnly)) {
        QString text = file.readAll();
        file.close();

        text = re_file.match(text).captured();
        QString name = re_name.match(text).captured().mid(1);
        name.chop(1);
        if (!name.isEmpty()) {
            return name;
        }
    }

    // find dock name in submenus file
    file.setFileName(QDir::homePath() + "/.fluxbox/submenus/appearance");

    if (!file.open(QFile::Text | QFile::ReadOnly)) {
        return {};
    }
    QString text = file.readAll();
    file.close();

    text = re_file.match(text).captured();

    QString name = re_name.match(text).captured().mid(1);
    name.chop(1);
    return name;
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
    for (int i = 0; i < apps.size(); ++i) {
        if (index == i) { // skip current element since we apply its style to all
            continue;
        }
        const QStringList app_info = QStringList(
            {apps.at(i).at(Info::App), apps.at(i).at(Info::Command), apps.at(i).at(Info::Tooltip),
             apps.at(i).at(Info::Icon), ui->comboSize->currentText(),
             ui->widgetBackground->palette().color(QWidget::backgroundRole()).name(),
             ui->widgetHoverBackground->palette().color(QWidget::backgroundRole()).name(),
             ui->widgetBorder->palette().color(QWidget::backgroundRole()).name(),
             ui->widgetHoverBorder->palette().color(QWidget::backgroundRole()).name(), apps.at(i).at(Info::Extra)});
        const quint8 width = ui->comboSize->currentText().section(QStringLiteral("x"), 0, 0).toUShort();
        const QSize size(width, width);
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
        list_icons.at(i)->setPixmap(list_icons.at(i)->pixmap()->scaled(size));
#else
        list_icons.at(i)->setPixmap(list_icons.at(i)->pixmap(Qt::ReturnByValue).scaled(size));
#endif
        (i < apps.size()) ? apps.replace(i, app_info) : apps.push_back(app_info);
        list_icons.at(i)->setStyleSheet(
            "background-color: " + ui->widgetBackground->palette().color(QWidget::backgroundRole()).name()
            + ";border: 4px solid " + ui->widgetBorder->palette().color(QWidget::backgroundRole()).name() + ";");
    }
    checkDoneEditing();
}

QString MainWindow::pickSlitLocation()
{
    auto *const pick = new PickLocation(slit_location, this);
    pick->exec();
    return pick->button;
}

void MainWindow::itemChanged()
{
    changed = true;
    updateAppList(index);
    checkDoneEditing();
    displayIcon(ui->buttonSelectApp->text(), index);
    list_icons.at(index)->setStyleSheet(list_icons.at(index)->styleSheet() + "border-width: 10px;");
    if (ui->checkApplyStyleToAll->isChecked()) {
        allItemsChanged();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    auto *clickedWidget = this->childAt(event->pos());

    if (clickedWidget == ui->widgetBackground || clickedWidget == ui->widgetHoverBackground
        || clickedWidget == ui->widgetBorder || clickedWidget == ui->widgetHoverBorder) {
        pickColor(clickedWidget);
    }

    if (!checkDoneEditing()) {
        return;
    }

    if (event->button() == Qt::LeftButton) {
        int i = ui->icons->layout()->indexOf(clickedWidget);
        int old_idx = index;
        if (i >= 0) {
            showApp(index = i, old_idx);
        }
    }
}

void MainWindow::updateAppList(int idx)
{
    const QStringList app_info = QStringList(
        {ui->buttonSelectApp->text(), ui->lineEditCommand->text(), ui->lineEditTooltip->text(),
         ui->buttonSelectIcon->text(), ui->comboSize->currentText(),
         ui->widgetBackground->palette().color(QWidget::backgroundRole()).name(),
         ui->widgetHoverBackground->palette().color(QWidget::backgroundRole()).name(),
         ui->widgetBorder->palette().color(QWidget::backgroundRole()).name(),
         ui->widgetHoverBorder->palette().color(QWidget::backgroundRole()).name(),
         ui->buttonSelectApp->property("extra_options").toString()});
    (idx < apps.size()) ? apps.replace(idx, app_info) : apps.push_back(app_info);
}

void MainWindow::addDockToMenu(const QString &file_name)
{
    cmd.run(R"(sed -i '/\[submenu\] (Docks)/a \\t\t\t[exec] ()" + dock_name + ") {" + file_name + "}' "
                + QDir::homePath() + "/.fluxbox/submenus/appearance",
            true);
}

void MainWindow::deleteDock()
{
    hide();

    const QString selectedDock
        = QFileDialog::getOpenFileName(nullptr, tr("Select dock to delete"), QDir::homePath() + "/.fluxbox/scripts",
                                       tr("Dock Files (*.mxdk);;All Files (*.*)"));

    if (!selectedDock.isEmpty()) {
        const QMessageBox::StandardButton confirmation = QMessageBox::question(
            nullptr, tr("Confirmation"), tr("Are you sure you want to delete %1?").arg(selectedDock),
            QMessageBox::Yes | QMessageBox::Cancel);

        if (confirmation == QMessageBox::Yes) {
            if (QFile::remove(selectedDock)) {
                cmd.run("sed -ni '\\|" + selectedDock + "|!p' " + QDir::homePath() + "/.fluxbox/submenus/appearance",
                        true);
                cmd.run(QStringLiteral("pkill wmalauncher"), true);
            } else {
                QMessageBox::warning(nullptr, tr("Error"), tr("Failed to delete the selected dock."));
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
    const QStringList possible_locations({"TopLeft", "TopCenter", "TopRight", "LeftTop", "RightTop", "LeftCenter",
                                          "RightCenter", "LeftBottom", "RightBottom", "BottomLeft", "BottomCenter",
                                          "BottomRight"});

    const QString selected_dock
        = QFileDialog::getOpenFileName(nullptr, tr("Select dock to move"), QDir::homePath() + "/.fluxbox/scripts",
                                       tr("Dock Files (*.mxdk);;All Files (*.*)"));
    if (selected_dock.isEmpty()) {
        setup();
        return;
    }

    QFile file(selected_dock);

    if (!file.open(QFile::Text | QFile::ReadOnly)) {
        qDebug() << "Could not open file:" << file.fileName();
        QMessageBox::warning(nullptr, tr("Could not open file"), tr("Could not open file"));
        setup();
        return;
    }
    QString text = file.readAll();
    file.close();

    // find location
    QRegularExpression re(possible_locations.join(QStringLiteral("|")));
    QRegularExpressionMatch match = re.match(text);
    if (match.hasMatch()) {
        slit_location = match.captured();
    }

    // select location
    slit_location = pickSlitLocation();

    // replace string
    re.setPattern(QStringLiteral("^sed -i.*"));
    re.setPatternOptions(QRegularExpression::MultilineOption);
    const QString newLine = "sed -i 's/^session.screen0.slit.placement:.*/session.screen0.slit.placement: "
                            + slit_location + "/' $HOME/.fluxbox/init";

    if (!file.open(QFile::Text | QFile::ReadWrite | QFile::Truncate)) {
        qDebug() << "Could not open file:" << file.fileName();
        QMessageBox::warning(nullptr, tr("Could not open file"), tr("Could not open file"));
        setup();
        return;
    }
    QTextStream out(&file);

    // add shebang and pkill wmalauncher
    out << "#!/bin/bash\n\n";
    out << "pkill wmalauncher\n\n";

    // remove if already existent
    text.remove(QStringLiteral("#!/bin/bash\n\n"))
        .remove(QStringLiteral("#!/bin/bash\n"))
        .remove(QStringLiteral("pkill wmalauncher\n\n"))
        .remove(QStringLiteral("pkill wmalauncher\n"));

    // if location line not found add it at the beginning
    if (!re.match(text).hasMatch()) {
        out << "#set up slit location\n" + newLine + "\n";
        out << text.remove(QStringLiteral("#set up slit location\n"));
    } else {
        out << text.replace(re, newLine);
    }
    file.close();
    cmd.run("pkill wmalauncher;" + file.fileName() + "&disown", true);
    setup();
    this->show();
}

// Move icon: pos -1 to shift left, +1 to shift right
void MainWindow::moveIcon(int pos)
{
    int newIndex = index + pos;
    if (newIndex < 0 || newIndex >= apps.size()) {
        return;
    }

    changed = true;
    // Swap the applications in the list
    apps.swapItemsAt(index, newIndex);

    // Swap the icons visually
    QLabel *currentIconLabel = list_icons.at(index);
    QLabel *newIconLabel = list_icons.at(newIndex);
    QPixmap currentIcon = currentIconLabel->pixmap(Qt::ReturnByValue);
    currentIconLabel->setPixmap(newIconLabel->pixmap(Qt::ReturnByValue));
    newIconLabel->setPixmap(currentIcon);

    // Update styles for the swapped icons
    QString currentStyle = currentIconLabel->styleSheet();
    currentIconLabel->setStyleSheet(newIconLabel->styleSheet());
    newIconLabel->setStyleSheet(currentStyle);

    // Update the current index
    index = newIndex;

    // Refresh the displayed application and highlight the current icon
    showApp(index, index - pos);
    displayIcon(ui->buttonSelectApp->text(), index);
    list_icons.at(index)->setStyleSheet(list_icons.at(index)->styleSheet() + "border-width: 10px;");

    checkDoneEditing();
}

void MainWindow::parseFile(QFile &file)
{
    blockComboSignals(true);
    parsing = true;

    const QStringList possible_locations({"TopLeft", "TopCenter", "TopRight", "LeftTop", "RightTop", "LeftCenter",
                                          "RightCenter", "LeftBottom", "RightBottom", "BottomLeft", "BottomCenter",
                                          "BottomRight"});

    QString line;
    const QSet<QString> knownOptions {QLatin1String("-d"),
                                      QLatin1String("--desktop-file"),
                                      QLatin1String("-c"),
                                      QLatin1String("--command"),
                                      QLatin1String("-i"),
                                      QLatin1String("--icon"),
                                      QLatin1String("-k"),
                                      QLatin1String("--background-color"),
                                      QLatin1String("-K"),
                                      QLatin1String("--hover-background-color"),
                                      QLatin1String("-b"),
                                      QLatin1String("--border-color"),
                                      QLatin1String("-B"),
                                      QLatin1String("--hover-border-color"),
                                      QLatin1String("-w"),
                                      QLatin1String("--window-size"),
                                      QLatin1String("--tooltip-text"),
                                      QLatin1String("-x"),
                                      QLatin1String("--exit-on-right-click")};

    while (!file.atEnd()) {
        line = file.readLine().trimmed();

        if (line.startsWith(QLatin1String("sed -i"))) { // assume it's a slit relocation sed command
            for (const QString &location : possible_locations) {
                if (line.contains(location)) {
                    slit_location = location;
                    break;
                }
            }
            continue;
        }
        if (line.startsWith(QLatin1String("wmalauncher"))) {
            ui->buttonSelectApp->setProperty("extra_options", QString());
            ui->lineEditTooltip->clear();
            line.remove(QRegularExpression(QStringLiteral("^wmalauncher")));
            line.remove(QRegularExpression(QStringLiteral("\\s*&(?:\\s*sleep.*)?$")));

            auto stripQuotes = [](const QString &value) {
                if (value.size() >= 2
                    && ((value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
                        || (value.startsWith(QLatin1Char('\'')) && value.endsWith(QLatin1Char('\''))))) {
                    return value.mid(1, value.size() - 2);
                }
                return value;
            };

            auto isKnownOption = [&](const QString &token) { return knownOptions.contains(stripQuotes(token)); };

            auto appendExtraOption = [&](const QString &option, const QString &value) {
                QString extra = ui->buttonSelectApp->property("extra_options").toString();
                if (!extra.isEmpty()) {
                    extra += QLatin1Char(' ');
                }
                extra += option;
                if (!value.isEmpty()) {
                    extra += QLatin1Char(' ') + value;
                }
                ui->buttonSelectApp->setProperty("extra_options", extra);
            };

            QRegularExpression tokenRe(QStringLiteral(R"((\"[^\"]*\"|'[^']*'|\S+))"));
            QStringList tokens;
            auto matchIt = tokenRe.globalMatch(line.trimmed());
            while (matchIt.hasNext()) {
                tokens << matchIt.next().captured(0);
            }

            for (int i = 0; i < tokens.size(); ++i) {
                const QString token = tokens.at(i);
                auto nextValue = [&](QString *out) {
                    if (i + 1 < tokens.size()) {
                        *out = stripQuotes(tokens.at(++i));
                    }
                };

                if (token == QLatin1String("-d") || token == QLatin1String("--desktop-file")) {
                    ui->radioDesktop->setChecked(true);
                    QString value;
                    nextValue(&value);
                    if (!value.isEmpty()) {
                        ui->buttonSelectApp->setText(value);
                    }
                    ui->buttonSelectIcon->setToolTip(QString());
                    ui->buttonSelectIcon->setStyleSheet(QStringLiteral("text-align: center; padding: 3px;"));
                } else if (token == QLatin1String("-c") || token == QLatin1String("--command")) {
                    ui->radioCommand->setChecked(true);
                    QString value;
                    nextValue(&value);
                    while (i + 1 < tokens.size() && !isKnownOption(tokens.at(i + 1))) {
                        value += QLatin1Char(' ') + stripQuotes(tokens.at(++i));
                    }
                    if (!value.isEmpty()) {
                        ui->lineEditCommand->setText(value);
                    }
                    ui->buttonSelectIcon->setStyleSheet(QStringLiteral("text-align: right; padding: 3px;"));
                } else if (token == QLatin1String("-i") || token == QLatin1String("--icon")) {
                    QString value;
                    nextValue(&value);
                    if (!value.isEmpty()) {
                        ui->buttonSelectIcon->setText(value);
                        ui->buttonSelectIcon->setToolTip(value);
                    }
                } else if (token == QLatin1String("-k") || token == QLatin1String("--background-color")) {
                    QString color;
                    nextValue(&color);
                    if (!color.isEmpty()) {
                        setColor(ui->widgetBackground, color);
                    }
                } else if (token == QLatin1String("-K") || token == QLatin1String("--hover-background-color")) {
                    QString color;
                    nextValue(&color);
                    if (!color.isEmpty()) {
                        setColor(ui->widgetHoverBackground, color);
                    }
                } else if (token == QLatin1String("-b") || token == QLatin1String("--border-color")) {
                    QString color;
                    nextValue(&color);
                    if (!color.isEmpty()) {
                        setColor(ui->widgetBorder, color);
                    }
                } else if (token == QLatin1String("-B") || token == QLatin1String("--hover-border-color")) {
                    QString color;
                    nextValue(&color);
                    if (!color.isEmpty()) {
                        setColor(ui->widgetHoverBorder, color);
                    }
                } else if (token == QLatin1String("-w") || token == QLatin1String("--window-size")) {
                    QString size;
                    nextValue(&size);
                    if (!size.isEmpty()) {
                        ui->comboSize->setCurrentIndex(ui->comboSize->findText(size + "x" + size));
                    }
                } else if (token == QLatin1String("--tooltip-text")) {
                    QString tooltip;
                    nextValue(&tooltip);
                    ui->lineEditTooltip->setText(tooltip);
                } else if (token == QLatin1String("-x") || token == QLatin1String("--exit-on-right-click")) {
                    // not used right now
                } else {
                    QString value;
                    if (i + 1 < tokens.size()) {
                        const QString possibleValue = tokens.at(i + 1);
                        if (!possibleValue.startsWith(QLatin1Char('-')) && !isKnownOption(possibleValue)) {
                            value = possibleValue;
                            ++i;
                        }
                    }
                    appendExtraOption(token, value);
                }
            }
            updateAppList(index);
            displayIcon(ui->buttonSelectApp->text(), index);
            index++;
        } else {
            file_content.append(line + "\n"); // add lines to the file_content, skipping wmalauncher lines
        }
    }
    ui->buttonSave->setDisabled(true);
    showApp(index = 0, -1);
    parsing = false;
}

void MainWindow::buttonSave_clicked()
{
    slit_location = pickSlitLocation();

    // create "~/.fluxbox/scripts" if it doesn't exist
    if (!QFileInfo::exists(QDir::homePath() + "/.fluxbox/scripts")) {
        QDir().mkpath(QDir::homePath() + "/.fluxbox/scripts");
    }

    bool new_file = false;
    if (!QFileInfo::exists(file_name)
        || QMessageBox::No
               == QMessageBox::question(this, tr("Overwrite?"), tr("Do you want to overwrite the dock file?"))) {
        file_name = QFileDialog::getSaveFileName(this, tr("Save file"), QDir::homePath() + "/.fluxbox/scripts",
                                                 tr("Dock Files (*.mxdk);;All Files (*.*)"));
        if (file_name.isEmpty()) {
            return;
        }
        if (!file_name.endsWith(QLatin1String(".mxdk"))) {
            file_name += QLatin1String(".mxdk");
        }
        new_file = true;
    }
    QFile file(file_name);
    cmd.run("cp " + file_name + " " + file_name + ".~", true);
    if (!file.open(QFile::Text | QFile::WriteOnly)) {
        qDebug() << "Could not open file:" << file.fileName();
        return;
    }
    QTextStream out(&file);
    if (file_content.isEmpty()) {
        // build and write string
        out << "#!/bin/bash\n\n";
        out << "pkill wmalauncher\n\n";
        out << "#set up slit location\n";
        out << "sed -i 's/^session.screen0.slit.placement:.*/session.screen0.slit.placement: " + slit_location
                   + "/' $HOME/.fluxbox/init\n\n";
        out << "fluxbox-remote restart; sleep 1\n\n";
        out << "#commands for dock launchers\n";
    } else {
        // add shebang and pkill wmalauncher
        out << "#!/bin/bash\n\n";
        out << "pkill wmalauncher\n\n";

        // remove if already existent
        file_content.remove(QStringLiteral("#!/bin/bash\n\n"))
            .remove(QStringLiteral("#!/bin/bash\n"))
            .remove(QStringLiteral("pkill wmalauncher\n\n"))
            .remove(QStringLiteral("pkill wmalauncher\n"));
        QRegularExpression re(QStringLiteral("^sed -i.*"), QRegularExpression::MultilineOption);
        QString newLine = "sed -i 's/^session.screen0.slit.placement:.*/session.screen0.slit.placement: "
                          + slit_location + "/' $HOME/.fluxbox/init";

        // if location line not found add it at the beginning
        if (!re.match(file_content).hasMatch()) {
            out << "#set up slit location\n" + newLine + "\n";
            out << file_content.remove(QStringLiteral("#set up slit location\n"));
        } else {
            out << file_content.replace(re, newLine);
        }
    }
    for (const auto &app : std::as_const(apps)) {
        const QString command = (app.at(Info::App).endsWith(QLatin1String(".desktop")))
                                    ? "--desktop-file " + quoteArgument(app.at(Info::App))
                                    : "--command " + app.at(Info::Command) + " --icon " + quoteArgument(app.at(Info::Icon));
        QString extraOptions = app.at(Info::Extra);
        if (!extraOptions.isEmpty() && !extraOptions.startsWith(QLatin1Char(' '))) {
            extraOptions.prepend(QLatin1Char(' '));
        }
        QString tooltipOption;
        if (!app.at(Info::Tooltip).isEmpty()) {
            tooltipOption = QStringLiteral(" --tooltip-text ") + quoteDouble(app.at(Info::Tooltip));
        }
        out << "wmalauncher " + command + " --background-color \"" + app.at(Info::BgColor)
                   + "\" --hover-background-color \"" + app.at(Info::BgHoverColor) + "\" --border-color \""
                   + app.at(Info::BorderColor) + "\" --hover-border-color \"" + app.at(Info::BorderHoverColor)
                   + "\" --window-size " + app.at(Info::Size).section(QStringLiteral("x"), 0, 0) + tooltipOption
                   + extraOptions + "& sleep 0.1\n";
    }
    file.close();
    QFile::setPermissions(file_name, QFlag(0x744));

    if (dock_name.isEmpty() || new_file) {
        dock_name = inputDockName();
    }

    if (dock_name.isEmpty()) {
        dock_name = QFileInfo(file).baseName();
    }

    if (!isDockInMenu(file.fileName())) {
        addDockToMenu(file.fileName());
    }

    QMessageBox::information(this, tr("Dock saved"),
                             tr("The dock has been saved.\n\n"
                                "To edit the newly created dock please select 'Edit an existing dock'."));
    QProcess::execute(QStringLiteral("pkill"), {"wmalauncher"});
    QProcess::startDetached(file.fileName(), {});
    index = 0;
    apps.clear();
    list_icons.clear();
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
    showApp(index, index - 1);
    blockComboSignals(false);
}

void MainWindow::buttonDelete_clicked()
{
    if (!apps.isEmpty()) {
        blockComboSignals(true);
        delete ui->icons->layout()->itemAt(index)->widget();
        list_icons.removeAt(index);
        apps.removeAt(index);
        blockComboSignals(false);
    }
    if (apps.isEmpty()) {
        index = 0;
        ui->buttonDelete->setEnabled(false);
        resetAdd();
    } else if (index == 0) {
        ui->buttonPrev->setEnabled(false);
        showApp(index, -1);
    } else {
        ui->buttonSave->setEnabled(true);
        showApp(--index, -1);
    }
    updateAppList(index);
    ui->buttonSave->setEnabled(true);
}

void MainWindow::resetAdd()
{
    ui->buttonSelectApp->setText(tr("Select..."));
    ui->buttonSelectApp->setProperty("extra_options", QString());
    ui->radioDesktop->click();
    emit ui->radioDesktop->toggled(true);
    ui->buttonAdd->setDisabled(true);
    ui->lineEditTooltip->clear();

    QString size;
    if (ui->checkApplyStyleToAll->isChecked() && index != 0) { // set style according to the first item
        size = apps.at(0).at(Info::Size);
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

void MainWindow::showApp(int idx, int old_idx)
{
    ui->radioCommand->blockSignals(true);
    ui->radioDesktop->blockSignals(true);

    ui->buttonSelectApp->setText(apps.at(idx).at(Info::App));
    if (apps.at(idx).at(0).endsWith(QLatin1String(".desktop")) || apps.at(idx).at(1).isEmpty()) {
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
        ui->lineEditCommand->setText(apps.at(idx).at(Info::Command));
        ui->buttonSelectIcon->setText(apps.at(idx).at(Info::Icon));
        ui->buttonSelectIcon->setToolTip(apps.at(idx).at(Info::Icon));
        ui->buttonSelectIcon->setStyleSheet(QStringLiteral("text-align: right; padding: 3px;"));
    }

    ui->lineEditTooltip->setText(apps.at(idx).at(Info::Tooltip));

    ui->radioCommand->blockSignals(false);
    ui->radioDesktop->blockSignals(false);

    blockComboSignals(true);
    ui->comboSize->setCurrentIndex(ui->comboSize->findText(apps.at(idx).at(Info::Size)));
    setColor(ui->widgetBackground, apps.at(idx).at(Info::BgColor));
    setColor(ui->widgetBorder, apps.at(idx).at(Info::BorderColor));
    setColor(ui->widgetHoverBackground, apps.at(idx).at(Info::BgHoverColor));
    setColor(ui->widgetHoverBorder, apps.at(idx).at(Info::BorderHoverColor));
    ui->buttonSelectApp->setProperty("extra_options", apps.at(idx).at(Info::Extra));
    blockComboSignals(false);

    // set buttons
    ui->buttonNext->setDisabled(idx == apps.size() - 1 || apps.size() == 1);
    ui->buttonPrev->setDisabled(idx == 0);
    ui->buttonAdd->setDisabled(apps.isEmpty());
    ui->buttonDelete->setEnabled(true);
    ui->buttonMoveLeft->setDisabled(idx == 0);
    ui->buttonMoveRight->setDisabled(idx == apps.size() - 1);

    if (old_idx != -1) {
        list_icons.at(old_idx)->setStyleSheet(list_icons.at(old_idx)->styleSheet() + "border-width: 4px;");
    }
    list_icons.at(idx)->setStyleSheet(list_icons.at(idx)->styleSheet() + "border-width: 10px;");
}

void MainWindow::buttonSelectApp_clicked()
{
    const QString selected
        = QFileDialog::getOpenFileName(nullptr, tr("Select .desktop file"), QStringLiteral("/usr/share/applications"),
                                       tr("Desktop Files (*.desktop)"));
    const QString file = QFileInfo(selected).fileName();
    if (!file.isEmpty()) {
        file_name = file;
        ui->buttonSelectApp->setText(file);
        ui->buttonAdd->setEnabled(true);
        ui->buttonSelectApp->setProperty("extra_options", QString()); // reset extra options when changing the app.
        ui->lineEditTooltip->clear();
        itemChanged();
    }
}

void MainWindow::editDock(const QString &file_arg)
{
    this->hide();

    QString selected_dock;
    if (!file_arg.isEmpty() && QFile::exists(file_arg)) {
        selected_dock = file_arg;
    } else {
        selected_dock
            = QFileDialog::getOpenFileName(nullptr, tr("Select a dock file"), QDir::homePath() + "/.fluxbox/scripts",
                                           tr("Dock Files (*.mxdk);;All Files (*.*)"));
    }

    if (!QFileInfo::exists(selected_dock)) {
        QMessageBox::warning(nullptr, tr("No file selected"),
                             tr("You haven't selected any dock file to edit.\nCreating a new dock instead."));
        return;
    }
    QFile file(selected_dock);
    if (!file.open(QFile::Text | QFile::ReadOnly)) {
        qDebug() << "Could not open file:" << file.fileName();
        QMessageBox::warning(nullptr, tr("Could not open file"),
                             tr("Could not open selected file.\nCreating a new dock instead."));
        return;
    }
    file_name = file.fileName();
    dock_name = getDockName(file_name);

    parseFile(file);
    file.close();
    ui->labelUsage->setText(tr("1. Edit applications one at a time using the Back and Next buttons\n"
                               "2. Add or delete applications as you like\n"
                               "3. When finished click Save"));
    this->show();
}

void MainWindow::newDock()
{
    this->show();
    apps.clear();
    list_icons.clear();

    resetAdd();
    ui->buttonSave->setEnabled(false);
    ui->buttonDelete->setEnabled(false);
}

void MainWindow::buttonPrev_clicked()
{
    blockComboSignals(true);
    updateAppList(index);
    int old_idx = index;
    showApp(--index, old_idx);
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
    QString selected = QFileDialog::getOpenFileName(nullptr, tr("Select icon"), default_folder,
                                                    tr("Icons (*.png *.jpg *.bmp *.xpm *.svg)"));
    QString file = QFileInfo(selected).fileName();
    if (!file.isEmpty()) {
        ui->buttonSelectIcon->setText(selected);
        ui->buttonSelectIcon->setToolTip(selected);
        ui->buttonSelectIcon->setStyleSheet(QStringLiteral("text-align: right; padding: 3px;"));
        updateAppList(index);
        displayIcon(ui->buttonSelectIcon->text(), index);
        list_icons.at(index)->setStyleSheet(list_icons.at(index)->styleSheet() + "border-width: 10px;");
    }
    if (!parsing) {
        checkDoneEditing();
    }
}

void MainWindow::lineEditCommand_textEdited()
{
    ui->buttonSave->setDisabled(ui->buttonNext->isEnabled());
    if (ui->buttonSelectIcon->text() != tr("Select icon...")) {
        ui->buttonNext->setEnabled(true);
        changed = true;
    } else {
        ui->buttonNext->setEnabled(false);
        return;
    }
    updateAppList(index);
    if (!parsing) {
        checkDoneEditing();
    }
}

void MainWindow::lineEditTooltip_textEdited()
{
    changed = true;
    updateAppList(index);
    if (!parsing) {
        checkDoneEditing();
    }
}

void MainWindow::buttonAdd_clicked()
{
    index++;
    resetAdd();
    list_icons.insert(index, new QLabel());
    const QStringList app_info {ui->buttonSelectApp->text(),
                                ui->lineEditCommand->text(),
                                ui->lineEditTooltip->text(),
                                ui->buttonSelectIcon->text(),
                                ui->comboSize->currentText(),
                                ui->widgetBackground->palette().color(QWidget::backgroundRole()).name(),
                                ui->widgetHoverBackground->palette().color(QWidget::backgroundRole()).name(),
                                ui->widgetBorder->palette().color(QWidget::backgroundRole()).name(),
                                ui->widgetHoverBorder->palette().color(QWidget::backgroundRole()).name(),
                                ui->buttonSelectApp->property("extra_options").toString()};
    apps.insert(index, app_info);
    ui->icons->insertWidget(index, list_icons.at(index));
    showApp(index, index - 1);
    itemChanged();
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
    if (index == apps.size() - 1) {
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
