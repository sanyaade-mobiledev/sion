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

#ifndef ATTRIBUTESMODEL_H
#define ATTRIBUTESMODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include <QVariant>

#include "treenode.h"

/**
  * This model feeds the attributes table. Formats the selected node's
  * attributes. It has to be noted that filter's don't have values for
  * the attributes.
  */
class MainWindow;
class AttributesModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit AttributesModel(MainWindow *windowP, QObject *parent = 0);
    
    int      rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int      columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    void update(bool forceUpdate);

public slots:
    
private:
    MainWindow      *m_windowP;
    FilterTreeNode  *m_lastSelectedFilterP; // optimize the model by keeping the last selected filter's plugins and attributes
    int             m_rows;
    QStringList     m_plugins;
    QStringList     m_attributePlugins;
    QStringList     m_attributes;

    inline void clearModel() {
        m_plugins.clear();
        m_attributePlugins.clear();
        m_attributes.clear();
        m_rows = 0;
    }
};

#endif // ATTRIBUTESMODEL_H
