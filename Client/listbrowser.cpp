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

#include "ui_listbrowser.h"
#include "listbrowser.h"

/**
  * Builds the item list model.
  */
ListBrowser::ListBrowser(bool editable, QStringList list, QWidget *parentP) :
    QDialog(parentP),
    ui(new Ui::ListBrowser) {

    ui->setupUi(this);
    ui->selectionEdit->setEnabled(editable);
    ui->selectionList->addItems(list);
    if (!list.isEmpty())
        ui->selectionList->item(0)->setSelected(true);

    setWindowTitle(tr("Select an item"));

    // the dialog is modal
    setModal(true);

    // connect to the list selection signal
    connect(ui->selectionList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(itemClicked(QListWidgetItem*)));
    connect(ui->selectionList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(itemDoubleClicked(QListWidgetItem*)));
}

/**
  * Deletes the item list browser.
  */
ListBrowser::~ListBrowser() {
    delete ui;
}

/**
  * Returns the user's last selected item.
  */
QString ListBrowser::getSelection() {
    return ui->selectionEdit->text();
}

/**
  * Sets the selected item's text to the user selection.
  */
void ListBrowser::itemClicked(QListWidgetItem *itemP) {
    ui->selectionEdit->setText(itemP->text());
}

/**
  * Emits an 'accept' signal if configured to do so. Derived classes
  * can override this to prevent the dialog from being closed on
  * double click.
  */
void ListBrowser::itemDoubleClicked(QListWidgetItem *itemP) {
    Q_UNUSED(itemP);

    if (acceptOnDoubleClick())
        emit accept();
}
