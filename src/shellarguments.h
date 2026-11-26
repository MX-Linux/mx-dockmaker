// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QStringList>

namespace ShellArguments
{
bool split(const QString &text, QStringList &arguments, QString &error, QStringList *spellings = nullptr);
QString quote(const QString &argument);

}
