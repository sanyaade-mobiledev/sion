/*
 * SION! Client interacts with SION! Server to let you virtually
 * organize content as you wish on your computer.
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

#ifndef DIRECTORYBROWSER_H
#define DIRECTORYBROWSER_H

#include <QListWidgetItem>

#include "listbrowser.h"
#include "serverproxy.h"

/**
  * This class specializes the ListBrowser class to prompt the user with
  * a list of directories to chose from. The list comes from the SION!Server.
  */
class DirectoryBrowser : public ListBrowser {
    Q_OBJECT

public:
    explicit DirectoryBrowser(QWidget *parentP, ServerProxy *serverP, QString directory);
    
public slots:
    void directories(QString directory, QStringList directories);
    void directoryDoubleClicked(QListWidgetItem *widgetP);
    void directoryClicked(QListWidgetItem *widgetP);

protected:
    virtual bool acceptOnDoubleClick() {
        return false;
    }

private:
    ServerProxy *m_serverP;
    QString m_directory;
};

#endif // DIRECTORYBROWSER_H
