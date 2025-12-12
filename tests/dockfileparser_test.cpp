/**********************************************************************
 *  dockfileparser_test.cpp
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

#include <QtTest/QtTest>

#include "dockconfiguration.h"
#include "dockfileparser.h"

class DockFileParserTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesMultipleEntriesAndDefaults();
};

void DockFileParserTest::parsesMultipleEntriesAndDefaults()
{
    const QString content = QStringLiteral(
        "wmalauncher --desktop-file app.desktop --background-color black --hover-background-color black "
        "--border-color white --hover-border-color white --window-size 64\n"
        "wmalauncher --command 'echo hi' --icon /usr/share/icons/hi.png --background-color black "
        "--hover-background-color black --border-color white --hover-border-color white\n");

    DockConfiguration configuration;
    DockFileParser parser;

    QVERIFY(parser.parseContent(content, configuration));
    QCOMPARE(configuration.getApplicationCount(), 2);

    const DockIconInfo first = configuration.getApplication(0);
    QCOMPARE(first.iconSize(), QSize(64, 64));
    QCOMPARE(first.size, QStringLiteral("64x64"));

    const DockIconInfo second = configuration.getApplication(1);
    QCOMPARE(second.iconSize(), QSize(48, 48)); // falls back to default size when not provided
    QCOMPARE(second.size, QStringLiteral("48x48"));
    QVERIFY(second.isValid());
}

QTEST_MAIN(DockFileParserTest)
#include "dockfileparser_test.moc"
