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

#ifndef ICONSVIEWMODEL_H
#define ICONSVIEWMODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include <QVariant>
#include <QTableView>
#include <QList>
#include <QIcon>

#include "common.h"
#include "treenode.h"

/**
  * This model feeds the icons view table. A simple non sortable model formatting the
  * selected filter's node children as an icon/text grid. The model adapts its number
  * of rows/columns to best fit the table's layout.
  */
class MainWindow;
class IconsViewModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit IconsViewModel(MainWindow *windowP, QObject *parent = 0);
    
    int      rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int      columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void    update(QTableView *tableP, bool forceUpdate);

public slots:
    
private:
    MainWindow      *m_windowP;
    FilterTreeNode  *m_lastSelectedFilterP; // optimize the model by keeping the last selected filter's plugins and attributes
    QStringList     m_nodes;
    QStringList     m_nodePaths;
    QList<QIcon>    m_nodeIcons;
    int             m_numColumns;
    int             m_numRows;

    inline void clearModel() {
        m_nodeIcons.clear();
        m_nodes.clear();
        m_nodePaths.clear();
        m_numColumns = 0;
        m_numRows = 0;
    }
};

#endif // ICONSVIEWMODEL_H
