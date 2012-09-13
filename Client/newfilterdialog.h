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

#ifndef NEWFILTERDIALOG_H
#define NEWFILTERDIALOG_H

#include <QLineEdit>
#include <QString>
#include <QDialog>

#include "ui_newfilterdialog.h"
#include "common.h"
#include "serverproxy.h"
#include "treenode.h"

/**
  * This dialog pops up when the user asks to create a new filter.
  * When instantiated, the dialog must be passed the server poxy, root node, the parentPath (full
  * path to the parent filter; or "" if no parent), the parent physical
  * directory (or "" if no parent), the list of available plugins, and the parent
  * window. The dialog will check the data validity based on the latter (see checkDataValidity).
  */
namespace Ui {
    class NewFilterDialog;
}

class NewFilterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewFilterDialog(ServerProxy *serverP, TreeNode *rootP, QString parentPath, QString parentDir, QStringList *pluginsP, QWidget *parentP);

    inline QString parentPath() {
        return ui->parentPathEdit->text();
    }

    inline QString filterName() {
        return ui->filterNameEdit->text();
    }

    inline QString filterPath() {
        QString path = parentPath();

        if (!path.isEmpty())
            path += VIRTUAL_PATH_SEPARATOR;

        return path + ui->filterNameEdit->text();
    }

    inline QString directory() {
        return ui->directoryEdit->text();
    }

    inline bool recursive() {
        return ui->recursiveCheck->isChecked();
    }

    QStringList plugins() {
        static QStringList plugins;

        plugins.clear();
        for (int i = 0; i < ui->pluginList->count(); i++)
            plugins.append(ui->pluginList->item(i)->text());

        return plugins;
    }

signals:

public slots:
    void removePlugin();
    void refreshPlugins();
    void browseDirectories();
    void done(int result);

protected:
    Ui::NewFilterDialog *ui;
    QStringList         m_plugins;
    ServerProxy         *m_serverP;
    TreeNode            *m_rootP;
    QString             m_parentDir;

    void populatePlugins();
    virtual bool checkDataValidity();
};

#endif // NEWFILTERDIALOG_H
