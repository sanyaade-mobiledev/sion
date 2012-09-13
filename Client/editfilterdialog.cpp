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

#include <QMessageBox>

#include "editfilterdialog.h"
#include "qfileinfoext.h"

/**
  * Builds the edit filter dialog.
  */
EditFilterDialog::EditFilterDialog(ServerProxy *serverP, TreeNode *rootP, QString parentPath, QString parentDir, QString filterName, QString directory, QStringList *pluginsP, QWidget *parentP) : NewFilterDialog(serverP, rootP, parentPath, parentDir, pluginsP, parentP) {
    // set and disable name and parent fields.
    ui->parentPathEdit->setText(parentPath);
    ui->parentPathEdit->setEnabled(false);
    ui->filterNameEdit->setText(filterName);
    ui->filterNameEdit->setEnabled(false);

    // sets the directory field
    ui->directoryEdit->setText(directory);

    setWindowTitle(tr("Edit Filter"));
}

/**
  * Checks the validity of the user input regarding the various filter modifications.
  */
bool EditFilterDialog::checkDataValidity() {
    QString parentDir;

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
                             tr("The specified directory is not under the parent filter's directory. No files would be 'filtered in'. Please select a physical directory located under the parent filter's directory."),
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

    // warn the user about the non recursive filters...
    if (!recursive()) {
        return QMessageBox::question(this,
                                     windowTitle(),
                                     tr("The new filter is not recursive. You won't be able to create children filters under this filter. Continue?"),
                                     QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
    }
    return true;
}
