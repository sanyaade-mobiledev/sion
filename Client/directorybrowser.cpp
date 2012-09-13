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

#include <QDir>

#include "mainwindow.h"
#include "ui_listbrowser.h"
#include "directorybrowser.h"

/**
  * Builds the directory browser dialog. A request is sent to the server via the proxy
  * to get the initial directory list.
  */
DirectoryBrowser::DirectoryBrowser(QWidget *parentP, ServerProxy *serverP, QString directory) :
    ListBrowser(false, QStringList(), parentP) {
    m_serverP = serverP;
    m_directory = directory;

    // connect the server signal
    connect(m_serverP, SIGNAL(directories(QString,QStringList)), this, SLOT(directories(QString,QStringList)));

    // connect to the list signal
    connect(ui->selectionList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(directoryDoubleClicked(QListWidgetItem*)));
    connect(ui->selectionList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(directoryClicked(QListWidgetItem*)));

    // display the directory
    ui->selectionEdit->setText(m_directory);

    // request directories under the server working directory
    serverP->requestDirectories(m_directory);
}

/**
  * This slot is invoked when the directory list is pushed by the server. It populates
  * the directories list view and root text edit.
  */
void DirectoryBrowser::directories(QString directory, QStringList directories) {
    m_directory = directory;

    // display the directory
    ui->selectionEdit->setText(m_directory);

    // update the list
    ui->selectionList->clear();
    ui->selectionList->addItems(directories);
}

/**
  * This slot is invoked when the user double clicks a directory. This generates
  * a request to the server for the list of directories under the double clicked one.
  * The double clicked item becomes the newly selected/root directory.
  */
void DirectoryBrowser::directoryDoubleClicked(QListWidgetItem *widgetP) {
    Q_UNUSED(widgetP);

    // request the new directory' entry list
    m_serverP->requestDirectories(m_directory);
}

/**
  * This slot is invoked when the user clicks a directory. This generates
  * The clicked item becomes the newly selected directory.
  */
void DirectoryBrowser::directoryClicked(QListWidgetItem *widgetP) {
    QDir dir(m_directory + QDir::separator() + widgetP->text());
    m_directory = dir.canonicalPath();

    // display the directory
    ui->selectionEdit->setText(m_directory);
}



