// SPDX-License-Identifier: GPL-3.0-or-later
#include "shellarguments.h"

#include <QCoreApplication>

// Read one shell word without expanding it. Double quotes only give backslash
// special meaning before $, backtick, double quote, backslash, or newline.
static bool readWord(QStringView text, qsizetype &pos, QString &value, QString &spelling)
{
    QChar delimiter;
    bool containsOperator = false;
    qsizetype start = pos;
    while (pos < text.size()) {
        const QChar ch = text.at(pos);
        if (delimiter.isNull() && QLatin1String(" \t\n").contains(ch)) {
            break;
        }
        ++pos;
        if (ch == QLatin1Char('\\') && delimiter != QLatin1Char('\'')) {
            if (pos == text.size()) {
                return false;
            }
            const QChar next = text.at(pos);
            if (delimiter.isNull() || QLatin1String("$`\"\\\n").contains(next)) {
                ++pos;
                if (next != QLatin1Char('\n')) {
                    value += next;
                } else {
                    // A dock stores one launcher per line. Omit the continuation
                    // from the saved spelling too, keeping the surrounding quotes.
                    spelling += text.sliced(start, pos - 2 - start);
                    start = pos;
                }
                continue;
            }
        }
        if (!delimiter.isNull() && ch == delimiter) {
            delimiter = QChar();
        } else if (delimiter.isNull() && (ch == QLatin1Char('\'') || ch == QLatin1Char('"'))) {
            delimiter = ch;
        } else {
            if (delimiter.isNull() && QLatin1String(";&|<>()").contains(ch)) {
                containsOperator = true;
            }
            value += ch;
        }
    }
    if (containsOperator) {
        spelling = ShellArguments::quote(value);
    } else {
        // Keep shell expansions distinct from quoted literals in the saved script.
        spelling += text.sliced(start, pos - start);
    }
    return delimiter.isNull();
}

bool ShellArguments::split(const QString &text, QStringList &arguments, QString &error, QStringList *spellings)
{
    arguments.clear();
    if (spellings) {
        spellings->clear();
    }
    error.clear();
    if (text.trimmed().isEmpty()) {
        return true;
    }

    qsizetype pos = 0;
    while (pos < text.size()) {
        const QChar ch = text.at(pos);
        if (QLatin1String(" \t\n").contains(ch)) {
            ++pos;
            continue;
        }
        if (ch == QLatin1Char('#')) {
            pos = text.indexOf(QLatin1Char('\n'), pos);
            if (pos < 0) {
                break;
            }
            continue;
        }
        if (QStringView(text).sliced(pos).startsWith(QLatin1String("\\\n"))) {
            pos += 2;
            continue;
        }
        QString value;
        QString spelling;
        if (!readWord(text, pos, value, spelling)) {
            arguments.clear();
            if (spellings) {
                spellings->clear();
            }
            error = QCoreApplication::translate("ShellArguments", "Unmatched quote or trailing backslash in command line.");
            return false;
        }
        arguments.append(value);
        if (spellings) {
            spellings->append(spelling);
        }
    }
    if (arguments.isEmpty()) {
        error = QCoreApplication::translate("ShellArguments", "Expected a command-line argument.");
        return false;
    }
    return true;
}

QString ShellArguments::quote(const QString &argument)
{
    QString escaped = argument;
    escaped.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + escaped + QLatin1Char('\'');
}
