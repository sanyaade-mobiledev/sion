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

#ifndef LISTBROWSER_H
#define LISTBROWSER_H

#include <QDialog>
#include <QListWidgetItem>

namespace Ui {
    class ListBrowser;
}

/**
  * Lets the use navigate through a list of items and pick one.
  */
class ListBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit ListBrowser(bool editable, QStringList list, QWidget *parentP);
    ~ListBrowser();

    virtual QString getSelection();

public slots:
    virtual void itemDoubleClicked(QListWidgetItem *itemP);
    virtual void itemClicked(QListWidgetItem *itemP);

protected:
    Ui::ListBrowser *ui;

    virtual bool acceptOnDoubleClick() {
        return true;
    }
};

#endif // LISTBROWSER_H
