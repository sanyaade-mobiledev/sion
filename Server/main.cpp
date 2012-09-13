/*
 * SION! Server meta-data / javascript indexing server.
 *
 * Copyright (C) Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Gilles Fabre <gilles.fabre@intel.com>
 */

#include <QtGui/QApplication>
#include "mainwindow.h"

#include "qfileext.h"

/**
  * Retrieves the user and password parameters from the command line
  * arguments, creates and shows the server window, then exec.
  */
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // set user and password
    QString user,
            pwd;

    // check the presence of "user=" and "pwd=" arguments
    QStringList arguments = app.arguments();
    for (QList<QString>::iterator i = arguments.begin(); i != arguments.end(); i++) {
        QString argument = *i;
        if (argument.startsWith("user=")) {
            user = argument.right(argument.length() - argument.indexOf("=") - 1);
        } else if (argument.startsWith("pwd=")) {
            pwd = argument.right(argument.length() - argument.indexOf("=") - 1);
        }
    }

    // user/pwd can't be empty
    if (user.isEmpty() || pwd.isEmpty()) {
        qDebug() << QObject::tr("user=<user> and/or pwd=<password> command line argument(s) missing");
        return -1;
    }

    MainWindow w;

    // set the user/pwd
    w.setUserAndPassword(user, pwd);
    w.show();

    // add the plugins directory to the path
    QString workingDir = QCoreApplication::applicationDirPath();
    QCoreApplication::addLibraryPath(workingDir);

    try {
        app.exec();
    } catch (const std::bad_alloc &) {
        qDebug() << "Out of memory";
        return -1; // exit the application
    }

    // clear the remote content cache
    QFileExt::emptyAndRemove(workingDir + QDir::separator() + CACHE_DIRECTORY);
}
