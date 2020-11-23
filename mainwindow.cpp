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

#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>
#include <QTimer>
#include <QSettings>

#include <about.h>
#include <cmd.h>
#include <sys/stat.h>
#include "mainwindow.h"
#include "picklocation.h"
#include "ui_mainwindow.h"
#include "version.h"

MainWindow::MainWindow(QWidget *parent, QString file) :
    QDialog(parent),
    ui(new Ui::MainWindow)
{
    qDebug() << "Program Version:" << VERSION;
    ui->setupUi(this);
    setWindowFlags(Qt::Window); // for the close, min and max buttons
    setup(file);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::isDockInMenu(const QString &file_name)
{
    if (dock_name.isEmpty()) return false;
    return getDockName(file_name) == dock_name;
}

void MainWindow::displayIcon(const QString &app_name, int location)
{
    QPalette pal = palette();
    QString icon;
    if (ui->buttonSelectApp->text().endsWith(".desktop")) {
        QFile file("/usr/share/applications/" + app_name);
        if(!file.open(QFile::Text | QFile::ReadOnly)) return;
        QString text = file.readAll();
        file.close();
        QRegularExpression re("^Icon=(.*)$", QRegularExpression::MultilineOption);
        icon = re.match(text).captured(1);
    } else {
        icon = ui->buttonSelectIcon->text();
    }
    int width = ui->comboSize->currentText().section("x", 0, 0).toInt();
    int height = width;
    QSize size(width, height);
    QPixmap pix = QPixmap(findIcon(icon)).scaled(size);
    if (location == list_icons.size()) {
        list_icons << new QLabel(this);
        ui->icons->addWidget(list_icons.last());
    }
    list_icons.at(location)->setPixmap(pix);
    list_icons.at(location)->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    list_icons.at(location)->setStyleSheet("background-color: " + ui->comboBgColor->currentText() + ";border: 4px solid " + ui->comboBorderColor->currentText() + ";");
}

bool MainWindow::checkDoneEditing()
{
    if (!apps.isEmpty()) ui->buttonDelete->setEnabled(true);

    if (ui->buttonSelectApp->text() != tr("Select...") || !ui->lineEditCommand->text().isEmpty()) {
        if (changed) ui->buttonSave->setEnabled(true);
        ui->buttonAdd->setEnabled(true);
        if (index != 0) ui->buttonPrev->setEnabled(true);
        if (index < apps.size() - 1 && apps.size() > 1) ui->buttonNext->setEnabled(true);
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
void MainWindow::setup(QString file)
{
    changed = false;
    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::cleanup);
    this->setWindowTitle("MX Dockmaker");
    ui->labelUsage->setText(tr("1. Add applications to the dock one at a time\n"
                               "2. Select a .desktop file or enter a command for the application you want\n"
                               "3. Select icon attributes for size, background (black is standard) and border\n"
                               "4. Press \"Add application\" to continue or \"Save\" to finish"));
    this->adjustSize();

    blockComboSignals(true);

    while (ui->icons->layout()->count() > 0) {
        delete ui->icons->layout()->itemAt(0)->widget();
    }
    ui->comboSize->setCurrentIndex(ui->comboSize->findText("48x48"));

    // "apricot", "mint" removed because not availalbe in Qt, might add back if needed
    QStringList color_list({"beige", "black", "blue", "brown", "cyan", "gray", "green",
                           "lavender", "lime", "magenta", "maroon", "navy", "olive",
                           "orange", "pink", "purple", "red", "teal", "white", "yellow"});

    file_content = "";

    ui->comboBgColor->addItems(color_list);
    ui->comboBorderColor->addItems(color_list);

    QSettings settings("MX-Linux", "mx-dockmaker");

    // Write configs if not there
    settings.setValue("BackgroundColor", settings.value("BackgroundColor", "black").toString());
    settings.setValue("FrameColor", settings.value("FrameColor", "white").toString());
    settings.setValue("Size", settings.value("Size", "48x48").toString());

    QString bg_color = settings.value("BackgroundColor", "black").toString();
    QString border_color = settings.value("FrameColor", "white").toString();
    QString size = settings.value("Size", "48x48").toString();

    ui->comboBgColor->setCurrentIndex(ui->comboBgColor->findText(bg_color));
    ui->comboBorderColor->setCurrentIndex(ui->comboBorderColor->findText(border_color));

    blockComboSignals(false);

    if (!file.isEmpty() && QFile::exists(file)) {
        editDock(file);
        return;
    }

    QMessageBox *mbox = new QMessageBox(nullptr);
    mbox->setText(tr("This tool allows you to create a new dock with one or more applications. You can also edit or delete a dock created earlier."));
    mbox->setIcon(QMessageBox::Question);
    mbox->setWindowTitle(tr("Operation mode"));
    mbox->addButton(tr("&Close"), QMessageBox::NoRole);
    mbox->addButton(tr("&Move"), QMessageBox::NoRole);
    mbox->addButton(tr("&Delete"), QMessageBox::NoRole);
    mbox->addButton(tr("&Edit"), QMessageBox::NoRole);
    mbox->addButton(tr("C&reate"), QMessageBox::NoRole);

    this->hide();

    switch (mbox->exec()) {
    case 1:
        moveDock();
        break;
    case 2:
        this->show();
        deleteDock();
        setup();
        break;
    case 3:
        this->show();
        editDock();
        break;
    case 4:
        newDock();
        break;
    default:
        QTimer::singleShot(0, qApp, &QGuiApplication::quit);
    }
    delete mbox;
}

QString MainWindow::findIcon(const QString &icon_name)
{
    if (QFileInfo::exists(icon_name)) return icon_name;

    const QStringList extList({".png", ".svg", ".xpm"});

    QString out = cmd.getCmdOut("xfconf-query -c xsettings -p /Net/IconThemeName", true);
    if (!out.isEmpty()) {
        QString dir = "/usr/share/icons/" + out;
        if (QFileInfo::exists(dir)) {
            for (const QString &ext : extList) {
                out = cmd.getCmdOut("find " + dir + " -iname " + icon_name + ext, true);
                if (!out.isEmpty()) {
                    QStringList files = out.split("\n");
                    return findLargest(files);
                } else {
                    out = cmd.getCmdOut("find " + dir + " /usr/share/icons/hicolor /usr/share/pixmaps -iname " + icon_name + ext, true);
                    if (!out.isEmpty()) {
                        QStringList files = out.split("\n");
                        return findLargest(files);
                    }
                }
            }
        }
    }
    return QString();
}

QString MainWindow::findLargest(const QStringList &files)
{
    qint64 max = 0;
    QString largest;
    for (const QString &file : files) {
        qint64 size = QFile(file).size();
        if (size >= max) {
            largest = file;
            max = size;
        }
    }
    return largest;
}

QString MainWindow::getDockName(const QString &file_name)
{
    return cmd.getCmdOut("grep " + QFileInfo(file_name).baseName() + " " + QDir::homePath() + "/.fluxbox/menu-mx|awk -F\"[()]\" '{print $2}'", true);
}

QString MainWindow::inputDockName()
{
    bool ok;
    QString text = QInputDialog::getText(nullptr, tr("Dock name"),
                                             tr("Enter the name to show in the Menu:"), QLineEdit::Normal,
                                             QString(), &ok);
    if (ok && !text.isEmpty()) return text;

    setup();
    return QString();
}

QString MainWindow::pickSlitLocation()
{
    PickLocation *pick = new PickLocation(slit_location, this);

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
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (!checkDoneEditing()) return;
    if (event->button() == Qt::LeftButton) {
        int i = ui->icons->layout()->indexOf(this->childAt(event->pos()));
        int old_idx = index;
        if (i >= 0) showApp(index = i, old_idx);
    }
}

void MainWindow::updateAppList(int idx)
{
    QStringList app_info = QStringList({ui->buttonSelectApp->text(), ui->lineEditCommand->text(), ui->buttonSelectIcon->text(),
                                        ui->comboSize->currentText(), ui->comboBgColor->currentText(), ui->comboBorderColor->currentText(),
                                        ui->buttonSelectApp->property("extra_options").toString()});
    (idx < apps.size()) ? apps.replace(idx, app_info) : apps.push_back(app_info);
}

void MainWindow::addDockToMenu(const QString &file_name)
{
    cmd.run("sed -i '/\\[submenu\\] (Docks)/a \\\\t\\t\\t[exec] (" + dock_name + ") {" +
            file_name + "}' " + QDir::homePath() + "/.fluxbox/menu-mx", true);
}

// cleanup environment when window is closed
void MainWindow::cleanup()
{

}

void MainWindow::deleteDock()
{
    this->hide();
    QString selected = QFileDialog::getOpenFileName(nullptr, tr("Select dock to delete"), QDir::homePath() + "/.fluxbox/scripts", tr("Dock Files (*.mxdk);;All Files (*.*)"));
    if (!selected.isEmpty() && QMessageBox::question(nullptr, tr("Confirmation"),
                                                     tr("Are you sure you want to delete %1?").arg(selected), tr("&Delete"), tr("&Cancel")) == 0) {
        QFile::remove(selected);
        cmd.run("sed -ni '\\|" + selected + "|!p' " + QDir::homePath() + "/.fluxbox/menu-mx", true);
        cmd.run("pkill wmalauncher", true);
    }
    this->show();
}

// block/unblock all relevant signals when loading stuff into GUI
void MainWindow::blockComboSignals(bool block)
{
    ui->comboSize->blockSignals(block);
    ui->comboBgColor->blockSignals(block);
    ui->comboBorderColor->blockSignals(block);
}

void MainWindow::moveDock()
{
    this->hide();
    const QStringList possible_locations({"TopLeft",    "TopCenter",    "TopRight",
                                          "LeftTop",                    "RightTop",
                                          "LeftCenter",                 "RightCenter",
                                          "LeftBottom",                 "RightBottom",
                                          "BottomLeft", "BottomCenter", "BottomRight"});

    QString selected_dock = QFileDialog::getOpenFileName(nullptr, tr("Select dock to move"), QDir::homePath() + "/.fluxbox/scripts", tr("Dock Files (*.mxdk);;All Files (*.*)"));
    if (selected_dock.isEmpty()) {
        setup();
        return;
    }

    QFile file(selected_dock);

    if(!file.open(QFile::Text | QFile::ReadOnly)) {
        qDebug() << "Could not open file:" << file.fileName();
        QMessageBox::warning(nullptr, tr("Could not open file"), tr("Could not open file"));
        setup();
        return;
    }
    QString text = file.readAll();
    file.close();

    // find location
    QRegularExpression re(possible_locations.join("|"));
    QRegularExpressionMatch match = re.match(text);
    if (match.hasMatch()) slit_location = match.captured();

    // select location
    slit_location = pickSlitLocation();

    // replace string
    re.setPattern("^sed -i.*");
    re.setPatternOptions(QRegularExpression::MultilineOption);
    QString new_line = "sed -i 's/^session.screen0.slit.placement:.*/session.screen0.slit.placement: " + slit_location + "/' $HOME/.fluxbox/init";

    if(!file.open(QFile::Text | QFile::ReadWrite | QFile::Truncate)) {
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
    text.remove("#!/bin/bash\n\n").remove("#!/bin/bash\n").remove("pkill wmalauncher\n\n").remove("pkill wmalauncher\n");

    // if location line not found add it at the beginning
    if (!re.match(text).hasMatch()) {
        out << "#set up slit location\n" + new_line + "\n";
        out << text.remove("#set up slit location\n");
    } else {
        out << text.replace(re, new_line);
    }

    file.close();
    cmd.run("pkill wmalauncher;" + file.fileName() + "&disown", true);
    setup();
    this->show();
}

void MainWindow::parseFile(QFile &file)
{
    blockComboSignals(true);
    parsing = true;

    const QStringList possible_locations({"TopLeft",    "TopCenter",    "TopRight",
                                          "LeftTop",                    "RightTop",
                                          "LeftCenter",                 "RightCenter",
                                          "LeftBottom",                 "RightBottom",
                                          "BottomLeft", "BottomCenter", "BottomRight"});

    QString line;
    QStringList options;

    while (!file.atEnd()) {
        line = file.readLine().trimmed();

        if (line.startsWith("sed -i")) { // assume it's a slit relocation sed command
            for (const QString &location : possible_locations) {
                if (line.contains(location)) {
                    slit_location = location;
                    break;
                }
            }
            continue;
        }
        if (line.startsWith("wmalauncher")) {
            ui->buttonSelectApp->setProperty("extra_options", QString());
            line.remove(QRegularExpression("^wmalauncher"));
            line.remove(QRegularExpression("\\s*&.*sleep.*$"));

            options = line.split(QRegularExpression(" --| -"));
            options.removeAll(QString());

            for (const QString &option: options) {
                QStringList tokens = option.split(" ");
                if ((tokens.at(0) == "d") | (tokens.at(0) == "desktop-file")) {
                    ui->radioDesktop->setChecked(true);
                    if (tokens.size() > 1) ui->buttonSelectApp->setText(tokens.at(1));
                    ui->buttonSelectIcon->setToolTip(QString());
                    ui->buttonSelectIcon->setStyleSheet("text-align: center; padding: 3px;");
                } else if ((tokens.at(0) ==  "c") | (tokens.at(0) == "command")) {
                    ui->radioCommand->setChecked(true);
                    if (tokens.size() > 1) ui->lineEditCommand->setText(tokens.mid(1).join(" ").trimmed());
                    ui->buttonSelectIcon->setStyleSheet("text-align: right; padding: 3px;");
                } else if ((tokens.at(0) == "i") | (tokens.at(0) == "icon")) {
                    if (tokens.size() > 1) {
                        ui->buttonSelectIcon->setText(tokens.mid(1).join(" "));
                        ui->buttonSelectIcon->setToolTip(tokens.mid(1).join(" "));
                    }
                } else if ((tokens.at(0) == "k") | (tokens.at(0) == "background-color")) {
                    if (tokens.size() > 1) ui->comboBgColor->setCurrentIndex(ui->comboBgColor->findText(tokens.at(1)));
                } else if ((tokens.at(0) == "b") | (tokens.at(0) == "border-color")) {
                    if (tokens.size() > 1) ui->comboBorderColor->setCurrentIndex(ui->comboBorderColor->findText(tokens.at(1)));
                } else if ((tokens.at(0) == "w") | (tokens.at(0) == "window-size")) {
                    if (tokens.size() > 1) ui->comboSize->setCurrentIndex(ui->comboSize->findText(tokens.at(1) + "x" + tokens.at(1)));
                } else if ((tokens.at(0) == "x") | (tokens.at(0) == "exit-on-right-click")) {
                    // not used right now
                } else { // other not handled options, add them as a propriety to the app button
                    ui->buttonSelectApp->setProperty("extra_options", ui->buttonSelectApp->property("extra_options").toString()
                                                     + ((tokens.at(0).length() > 1) ? " --" : " -") + tokens.join(" "));
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


// Next button clicked
void MainWindow::on_buttonSave_clicked()
{
    slit_location = pickSlitLocation();

    // create "~/.fluxbox/scripts" if it doesn't exist
    if(!QFileInfo::exists(QDir::homePath() + "/.fluxbox/scripts")) {
        QDir dir;
        dir.mkpath(QDir::homePath() + "/.fluxbox/scripts");
    }

    bool new_file = false;
    if(!QFileInfo::exists(file_name) || QMessageBox::No == QMessageBox::question(this, tr("Overwrite?"), tr("Do you want to overwrite the dock file?"))) {
        file_name = QFileDialog::getSaveFileName(this, tr("Save file"), QDir::homePath() + "/.fluxbox/scripts", tr("Dock Files (*.mxdk);;All Files (*.*)"));
        if (file_name.isEmpty()) return;
        if (!file_name.endsWith(".mxdk")) file_name += ".mxdk";
        new_file = true;
    }
    QFile file(file_name);
    cmd.run("cp " + file_name + " " + file_name + ".~", true);
    if(!file.open(QFile::Text | QFile::WriteOnly)) {
        qDebug() << "Could not open file:" << file.fileName();
        return;
    }
    QTextStream out(&file);

    if (file_content.isEmpty()) {
        // build and write string
        out << "#!/bin/bash\n\n";
        out << "pkill wmalauncher\n\n";
        out << "#set up slit location\n";
        out << "sed -i 's/^session.screen0.slit.placement:.*/session.screen0.slit.placement: " + slit_location + "/' $HOME/.fluxbox/init\n\n";
        out << "fluxbox-remote restart; sleep 1\n\n";
        out << "#commands for dock launchers\n";
    } else {
        // add shebang and pkill wmalauncher
        out << "#!/bin/bash\n\n";
        out << "pkill wmalauncher\n\n";

        // remove if already existent
        file_content.remove("#!/bin/bash\n\n").remove("#!/bin/bash\n").remove("pkill wmalauncher\n\n").remove("pkill wmalauncher\n");

        QRegularExpression re("^sed -i.*", QRegularExpression::MultilineOption);
        QString new_line = "sed -i 's/^session.screen0.slit.placement:.*/session.screen0.slit.placement: " + slit_location + "/' $HOME/.fluxbox/init";

        // if location line not found add it at the beginning
        if (!re.match(file_content).hasMatch()) {
            out << "#set up slit location\n" + new_line + "\n";
            out << file_content.remove("#set up slit location\n");
        } else {
            out << file_content.replace(re, new_line);
        }
    }

    for (int i = 0; i < apps.size(); ++i) {
        QString command = (apps.at(i).at(0).endsWith(".desktop")) ? "--desktop-file " + apps.at(i).at(0) : "--command " + apps.at(i).at(1) + " --icon " + apps.at(i).at(2);
        out << "wmalauncher " + command + " --background-color " + apps.at(i).at(4) + " --border-color " +
                  apps.at(i).at(5) + " --window-size " + apps.at(i).at(3).section("x", 0, 0) + apps.at(i).at(6) + "& sleep 0.1\n";
    }
    file.close();
    chmod(file_name.toUtf8(), 00744);

    if (dock_name.isEmpty() || new_file) dock_name = inputDockName();
    if (dock_name.isEmpty()) dock_name = QFileInfo(file).baseName();

    if (!isDockInMenu(file.fileName())) {
        addDockToMenu(file.fileName());
    }

    QMessageBox::information(this, tr("Dock saved"), tr("The dock has been saved.\n\n"
                                                        "To edit the newly created dock please select 'Edit an existing dock'."));

    cmd.run("pkill wmalauncher;" + file.fileName() + "&disown", true);

    index = 0;
    apps.clear();
    list_icons.clear();
    resetAdd();
    ui->buttonSave->setEnabled(false);
    ui->buttonDelete->setEnabled(false);
    setup();
}


// About button clicked
void MainWindow::on_buttonAbout_clicked()
{
    this->hide();
    displayAboutMsgBox( tr("About %1") + "MX Dockmaker",
                       "<p align=\"center\"><b><h2>MX Dockmaker</h2></b></p><p align=\"center\">" +
                       tr("Version: ") + VERSION + "</p><p align=\"center\"><h3>" +
                       tr("Description goes here") +
                       "</h3></p><p align=\"center\"><a href=\"http://mxlinux.org\">http://mxlinux.org</a><br /></p><p align=\"center\">" +
                       tr("Copyright (c) MX Linux") + "<br /><br /></p>",
                        "/usr/share/doc/mx-dockmaker/license.html", tr("%1 License").arg(this->windowTitle()), false);

    this->show();
}

// Help button clicked
void MainWindow::on_buttonHelp_clicked()
{
    QString url = "https://mxlinux.org/wiki/help-files/help-mx-dockmaker/";
    displayDoc(url, tr("%1 Help").arg(this->windowTitle()), false);
}


void MainWindow::on_comboSize_currentIndexChanged(const QString)
{
    itemChanged();
}

void MainWindow::on_comboBgColor_currentIndexChanged(const QString)
{
    itemChanged();
}

void MainWindow::on_comboBorderColor_currentIndexChanged(const QString)
{
    itemChanged();
}


void MainWindow::on_buttonNext_clicked()
{
    blockComboSignals(true);
    updateAppList(index);
    index++;
    showApp(index, index - 1);
    blockComboSignals(false);
}

void MainWindow::on_buttonDelete_clicked()
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
    ui->radioDesktop->toggled(true);
    ui->buttonAdd->setDisabled(true);

    QSettings settings("MX-Linux", "mx-dockmaker");

    QString bg_color = settings.value("BackgroundColor", "black").toString();
    QString border_color = settings.value("FrameColor", "white").toString();
    QString size = settings.value("Size", "48x48").toString();

    blockComboSignals(true);
    ui->comboSize->setCurrentIndex(ui->comboSize->findText(size));
    ui->comboBgColor->setCurrentIndex(ui->comboBgColor->findText(bg_color));
    ui->comboBorderColor->setCurrentIndex(ui->comboBorderColor->findText(border_color));
    blockComboSignals(false);

    ui->buttonSelectIcon->setToolTip(QString());
    ui->buttonSelectIcon->setStyleSheet("text-align: center; padding: 3px;");
}

void MainWindow::showApp(int idx, int old_idx)
{
    ui->radioCommand->blockSignals(true);
    ui->radioDesktop->blockSignals(true);

    ui->buttonSelectApp->setText(apps.at(idx).at(0));
    if (apps.at(idx).at(0).endsWith(".desktop") || apps.at(idx).at(1).isEmpty()) {
        ui->radioDesktop->setChecked(true);
        ui->lineEditCommand->clear();
        ui->buttonSelectApp->setEnabled(true);
        ui->lineEditCommand->setEnabled(false);
        ui->buttonSelectIcon->setEnabled(false);
        ui->buttonSelectIcon->setText(tr("Select icon..."));
        ui->buttonSelectIcon->setStyleSheet("text-align: center; padding: 3px;");
    } else {
        ui->radioCommand->setChecked(true);
        ui->buttonSelectApp->setText(tr("Select..."));
        ui->buttonSelectApp->setEnabled(false);
        ui->lineEditCommand->setEnabled(true);
        ui->buttonSelectIcon->setEnabled(true);
        ui->lineEditCommand->setText(apps.at(idx).at(1));
        ui->buttonSelectIcon->setText(apps.at(idx).at(2));
        ui->buttonSelectIcon->setToolTip(apps.at(idx).at(2));
        ui->buttonSelectIcon->setStyleSheet("text-align: right; padding: 3px;");
    }

    ui->radioCommand->blockSignals(false);
    ui->radioDesktop->blockSignals(false);

    blockComboSignals(true);
    ui->comboSize->setCurrentIndex(ui->comboSize->findText(apps.at(idx).at(3)));
    ui->comboBgColor->setCurrentIndex(ui->comboBgColor->findText(apps.at(idx).at(4)));
    ui->comboBorderColor->setCurrentIndex(ui->comboBorderColor->findText(apps.at(idx).at(5)));
    ui->buttonSelectApp->setProperty("extra_options", apps.at(idx).at(6));
    blockComboSignals(false);

    // set buttons
    ui->buttonNext->setDisabled(idx == apps.size() - 1 || apps.size() == 1);
    ui->buttonPrev->setDisabled(idx == 0);
    ui->buttonAdd->setDisabled(apps.isEmpty());
    ui->buttonDelete->setEnabled(true);
    ui->buttonMoveLeft->setDisabled(idx == 0);
    ui->buttonMoveRight->setDisabled(idx == apps.size() - 1);

    if (old_idx != -1) list_icons.at(old_idx)->setStyleSheet(list_icons.at(old_idx)->styleSheet() + "border-width: 4px;");
    list_icons.at(idx)->setStyleSheet(list_icons.at(idx)->styleSheet() + "border-width: 10px;");
}


void MainWindow::on_buttonSelectApp_clicked()
{
    QString selected = QFileDialog::getOpenFileName(nullptr, tr("Select .desktop file"), "/usr/share/applications", tr("Desktop Files (*.desktop)"));
    QString file = QFileInfo(selected).fileName();
    if (!file.isEmpty()) {
        file_name = file;
        ui->buttonSelectApp->setText(file);
        ui->buttonAdd->setEnabled(true);
        ui->buttonSelectApp->setProperty("extra_options", QString()); // reset extra options when changing the app.
        itemChanged();
    }
}

void MainWindow::editDock(QString file_arg)
{
    this->hide();

    QString selected_dock;
    if (!file_arg.isEmpty() && QFile::exists(file_arg)) {
        selected_dock = file_arg;
    } else {
        selected_dock = QFileDialog::getOpenFileName(nullptr, tr("Select a dock file"), QDir::homePath() + "/.fluxbox/scripts", tr("Dock Files (*.mxdk);;All Files (*.*)"));
    }
    if (!QFileInfo::exists(selected_dock)) {
        QMessageBox::warning(nullptr, tr("No file selected"), tr("You haven't selected any dock file to edit.\nCreating a new dock instead."));
        return;
    }
    QFile file(selected_dock);
    if(!file.open(QFile::Text | QFile::ReadOnly)) {
        qDebug() << "Could not open file:" << file.fileName();
        QMessageBox::warning(nullptr, tr("Could not open file"), tr("Could not open selected file.\nCreating a new dock instead."));
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
    dock_name = inputDockName();
    this->show();
    apps.clear();
    list_icons.clear();

    resetAdd();
    ui->buttonSave->setEnabled(false);
    ui->buttonDelete->setEnabled(false);
}

void MainWindow::on_buttonPrev_clicked()
{
    blockComboSignals(true);
    updateAppList(index);
    int old_idx = index;
    showApp(--index, old_idx);
    blockComboSignals(false);
}

void MainWindow::on_radioDesktop_toggled(bool checked)
{
    ui->lineEditCommand->clear();
    ui->buttonSelectApp->setEnabled(checked);
    ui->lineEditCommand->setEnabled(!checked);
    ui->buttonSelectIcon->setEnabled(!checked);
    if (checked) {
        ui->buttonSelectIcon->setText(tr("Select icon..."));
        ui->buttonSelectIcon->setStyleSheet("text-align: center; padding: 3px;");
    } else {
        ui->buttonSelectApp->setText(tr("Select..."));
    }
    if (!parsing) checkDoneEditing();
}

void MainWindow::on_buttonSelectIcon_clicked()
{
    ui->buttonSave->setDisabled(ui->buttonNext->isEnabled());
    QString selected = QFileDialog::getOpenFileName(nullptr, tr("Select icon"), "/usr/share/icons/Moka", tr("Icons (*.png *.jpg *.bmp *.xpm *.svg)"));
    QString file = QFileInfo(selected).fileName();
    if (!file.isEmpty()) {
        ui->buttonSelectIcon->setText(selected);
        ui->buttonSelectIcon->setToolTip(selected);
        ui->buttonSelectIcon->setStyleSheet("text-align: right; padding: 3px;");
        updateAppList(index);
        displayIcon(ui->buttonSelectIcon->text(), index);
        list_icons.at(index)->setStyleSheet(list_icons.at(index)->styleSheet() + "border-width: 10px;");
    }
    if (!parsing) checkDoneEditing();
}

void MainWindow::on_lineEditCommand_textEdited(const QString)
{
    qDebug() << "lineEditCommand changed";
    ui->buttonSave->setDisabled(ui->buttonNext->isEnabled());
    if(ui->buttonSelectIcon->text() != tr("Select icon...")) {
        ui->buttonSelectApp->setProperty("extra_options", QString()); // reset extra options when changing the command
        ui->buttonNext->setEnabled(true);
        changed = true;
    } else {
        ui->buttonNext->setEnabled(false);
        return;
    }
    if (!parsing) checkDoneEditing();
}


void MainWindow::on_buttonAdd_clicked()
{
    index++;
    resetAdd();
    list_icons.insert(index, new QLabel());
    QStringList app_info = QStringList({ui->buttonSelectApp->text(), ui->lineEditCommand->text(), ui->buttonSelectIcon->text(),
                                        ui->comboSize->currentText(), ui->comboBgColor->currentText(), ui->comboBorderColor->currentText(),
                                        ui->buttonSelectApp->property("extra_options").toString()});
    apps.insert(index, app_info);
    ui->icons->insertWidget(index, list_icons.at(index));
    itemChanged();
}


void MainWindow::on_buttonMoveLeft_clicked()
{
    if(index == 0) return;
    changed = true;

    apps.swap(index, index - 1);
    QPixmap map = *list_icons.at(index)->pixmap();
    list_icons.at(index)->setPixmap(*list_icons.at(index - 1)->pixmap());
    list_icons.at(index - 1)->setPixmap(map);
    list_icons.at(index)->setStyleSheet(list_icons.at(index - 1)->styleSheet());

    --index;
    showApp(index, index - 1);
    displayIcon(ui->buttonSelectApp->text(), index);
    list_icons.at(index)->setStyleSheet(list_icons.at(index)->styleSheet() + "border-width: 10px;");

    checkDoneEditing();
}

void MainWindow::on_buttonMoveRight_clicked()
{
    if(index == apps.size() - 1) return;
    changed = true;

    apps.swap(index, index + 1);
    QPixmap map = *list_icons.at(index)->pixmap();
    list_icons.at(index)->setPixmap(*list_icons.at(index + 1)->pixmap());
    list_icons.at(index + 1)->setPixmap(map);
    list_icons.at(index)->setStyleSheet(list_icons.at(index + 1)->styleSheet());

    ++index;
    showApp(index, index - 1);
    displayIcon(ui->buttonSelectApp->text(), index);
    list_icons.at(index)->setStyleSheet(list_icons.at(index)->styleSheet() + "border-width: 10px;");

    checkDoneEditing();
}
