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

#include <QFileDialog>
#include <QString>
#include <QMessageBox>

#include "ui_newfilterdialog.h"
#include "newfilterdialog.h"
#include "qfileinfoext.h"
#include "directorybrowser.h"

/**
  * New Filter Dialog constructor. This dialog is used to prompt the user for the necessary parameters in order
  * to create a new filter:
  *       Parent filter path
  *       Filter path
  *       Associated Physical directory
  *       Plugins
  *       Recursive watching in the physical directory
  */
NewFilterDialog::NewFilterDialog(ServerProxy *serverP, TreeNode *rootP, QString parentPath, QString parentDir, QStringList *pluginsP, QWidget *parentP) :
    QDialog(parentP),
    ui(new Ui::NewFilterDialog) {
    m_serverP = serverP;
    m_rootP = rootP;
    m_parentDir = parentDir;

    // builds the ui
    ui->setupUi(this);

    // presets the parent path and dir
    ui->parentPathEdit->setText(parentPath);
    ui->directoryEdit->setText(parentDir);

    // populate the plugins list
    m_plugins = *pluginsP;
    populatePlugins();

    // the dialog is modal
    setModal(true);
}

/**
  * Populate the list of the available plugins for the new filter.
  */
void NewFilterDialog::populatePlugins() {
    ui->pluginList->clear();
    for (QStringList::iterator i = m_plugins.begin(); i != m_plugins.end(); i++)
        ui->pluginList->addItem(*i);
}

/**
  * Removes the selected plugin from the list.
  */
void NewFilterDialog::removePlugin() {
    ui->pluginList->takeItem(ui->pluginList->currentRow());
}

/**
  * Repopulates the plugins list with the original list of available plugins.
  */
void NewFilterDialog::refreshPlugins() {
    populatePlugins();
}

/**
  * Prompts the user with a directory browser dialog to let her select
  * a physical directory to associate with the new filter.
  */
void NewFilterDialog::browseDirectories() {
    DirectoryBrowser browser(this, m_serverP, ui->directoryEdit->text());
    if (browser.exec() == QDialog::Accepted)
        ui->directoryEdit->setText(browser.getSelection());
}

/**
  * The user has pressed one of the dialog buttons, if the user decided to actually
  * create the new filter, the user input will be checked. If the user input is ok, or
  * if the latter cancelled the filter creation, the dialog will be closed.
  */
void NewFilterDialog::done(int result) {
    if(result == QDialog::Accepted) {
        // ok was pressed
        if(checkDataValidity()) {
            QDialog::done(result);
            return;
        }
    } else {
        // cancel, close or exc was pressed
        QDialog::done(result);
        return;
    }
}

/**
  * Checks the validity of the user input regarding the various filter creation constraints:
  * the path of the filter must be unique, the watched physical directory must be under the parent's
  * one (if any), etc. Returns true if the data is valid, false else.
  */
bool NewFilterDialog::checkDataValidity() {
    // check that the name is unique in the parent
    if (m_rootP->findNode(filterPath())) {
        QMessageBox::warning(this,
                             windowTitle(),
                             tr("This directory already exists. Please specify a different name or parent."),
                             QMessageBox::Close);
        return false;
    }

    // check that the dir exists and is under the parent dir...
    QString directory = this->directory();
    if (directory.isEmpty()) {
        QMessageBox::warning(this,
                             windowTitle(),
                             tr("The watched directory is empty. Please set the associated physical directory."),
                             QMessageBox::Close);
        return false;
    }

    QFileInfoExt dirInfo(directory);
    if (!dirInfo.isDir()) {
        QMessageBox::warning(this,
                             windowTitle(),
                             tr("The specified directory is invalid (not a directory). Please specify a valid physical directory."),
                             QMessageBox::Close);
        return false;
    }

    QFileInfoExt parentDirInfo(m_parentDir);
    if (!m_parentDir.isEmpty() && !dirInfo.canonicalPath().startsWith(parentDirInfo.canonicalPath())) {
        QMessageBox::warning(this,
                             windowTitle(),
                             tr("The specified directory is not under the parent filter's directory. No files would be 'filtered in'. Please select a physical directory located under the parent filter's directory or select a different parent filter."),
                             QMessageBox::Close);
        return false;
    }

    // check that the plugins list is not empty
    if (!ui->pluginList->count()) {
        QMessageBox::warning(this,
                             windowTitle(),
                             tr("Please specify at least one plugin (else no file would be 'filtered in'). Hit 'Refresh' to reset the list."),
                             QMessageBox::Close);
        return false;
    }

    // check that the filter path is correct
    if (filterName().isEmpty()) {
        QMessageBox::warning(this,
                             windowTitle(),
                             tr("The new filter name must be non empty and unique accross the parent's children."),
                             QMessageBox::Close);
        return false;
    }

    // warn the user about the non recursive filters...
    if (!recursive()) {
        return QMessageBox::question(this,
                                     windowTitle(),
                                     tr("The new filter is not recursive. You won't be able to create children filters under this filter. Continue?"),
                                     QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
    }
    return true;
}
