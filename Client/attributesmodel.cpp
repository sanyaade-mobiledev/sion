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

#include "attributesmodel.h"
#include "treenode.h"
#include "mainwindow.h"

AttributesModel::AttributesModel(MainWindow *windowP, QObject *parent) : QAbstractTableModel(parent) {
  m_windowP = windowP;
  m_rows = 0;
}

/**
  * Returns the number of attributes for the selected node. Sums
  * the number of attributes for every plugin of the selected
  * node's filter (the node itself if a filter or its parent). This is
  * computed in the update method.
  */
int AttributesModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);

    return m_rows;
}

/**
  * Returns the number of columns for the selected filter. A filter
  * has 4 columns: plugin, attribute, attribute class, attribute tip.
  * A file has 5 columns: plugin, attribute, attribute value, attribute
  * class, attribute tip.
  */
int AttributesModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);

    // get the selected node
    TreeNode *selNodeP = m_windowP->getSelectedNode();
    if (!selNodeP)
        return 0;

    return selNodeP->isFilter() ? 4 : 5;
}

/**
  * Returns the data for the attributes table.
  */
QVariant AttributesModel::data(const QModelIndex &index, int role) const {
    QString result;

    // we don't do anything else than Display
    if (role != Qt::DisplayRole)
        return result;

    // get the selected node
    TreeNode *selNodeP = m_windowP->getSelectedNode();
    if (!selNodeP)
        return result;

    // get the associated filter
    FilterTreeNode *filterP = NULL;
    if (selNodeP->isFilter())
        filterP = (FilterTreeNode *)selNodeP;
    else
        filterP = (FilterTreeNode *)selNodeP->parent();

    if (!filterP)
        return result; // this shouldn't happen

    // we moved to a different filter, can't return a valid value
    if (m_lastSelectedFilterP != filterP)
        return result;

    // get the attribute number
    int row = index.row();
    int column = index.column();
    if (row == -1 || column == -1)
        return result;

    QString plugin = m_attributePlugins[row];
    QString attribute = m_attributes[row];

    // get the appropriate data depending on the column
    switch (column) {
        case 0:
            // the plugin name.
            result = plugin;
            break;

        case 1:
            // the attribute name.
            result = attribute;
            break;

        case 2:
            // the attribute value or class depending on the selected node type
            if (selNodeP->isFilter())
                result = filterP->getAttributeClass(plugin, attribute);
            else
                result = filterP->getAttributeValue(selNodeP->getPath(), attribute);
            break;

        case 3:
            // the attribute class or tip depending on the selected node type
            if (selNodeP->isFilter())
                result = filterP->getAttributeTip(plugin, attribute);
            else
                result = filterP->getAttributeClass(plugin, attribute);
            break;

        case 4:
            // the attribute tip (the selected node MUST be a file)
            result = filterP->getAttributeTip(plugin, attribute);
            break;
    }

    return result;
}

/**
  * Returns the header data for the attributes table.
  */
QVariant AttributesModel::headerData(int section, Qt::Orientation orientation, int role) const {
    QVariant result;

    // we don't do anything else than Display
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return result;

    // get the selected node
    TreeNode *selNodeP = m_windowP->getSelectedNode();
    if (!selNodeP)
        return result;

    // set the column header depending on the selected node type
    switch (section) {
        case 0:
            result.setValue(tr("Plugin"));
            break;

        case 1:
            result.setValue(tr("Attribute"));
            break;

        case 2:
            if (selNodeP->isFilter())
                result.setValue(tr("Class"));
            else
                result.setValue(tr("Value"));
            break;

        case 3:
            if (selNodeP->isFilter())
                result.setValue(tr("Tip"));
            else
                result.setValue(tr("Class"));
            break;

        case 4:
            result.setValue(tr("Tip"));
            break;
    }

    return result;
}

/**
  * Forces an update of the attributes table.
  */
void AttributesModel::update(bool forceUpdate) {
    // get the selected node
    TreeNode *selNodeP = m_windowP->getSelectedNode();
    if (!selNodeP) {
        clearModel();
        reset();
        return;
    }

    // get the associated filter
    FilterTreeNode *filterP = NULL;
    if (selNodeP->isFilter())
        filterP = (FilterTreeNode *)selNodeP;
    else
        filterP = (FilterTreeNode *)selNodeP->parent();

    // we don't know the filter, can't display attributes
    if (!filterP) {
        clearModel();
        reset();
        return;
    }

    // we didn't move to a different filter and nothing changed, return the last values
    if (filterP == m_lastSelectedFilterP &&
        !forceUpdate) {
        reset();
        return;
    }

    // keep the ref of the last selected filter
    m_lastSelectedFilterP = filterP;

    // there's a row per attribute
    m_attributes.clear();
    m_attributePlugins.clear();
    m_plugins = filterP->getPlugins();
    for (int i = 0; i < m_plugins.count(); i++) {
        QString plugin = m_plugins[i];
        QStringList attributes = filterP->getPluginAttributes(plugin);
        for (int j = 0; j < attributes.count(); j++)
            m_attributePlugins.append(plugin);
        m_attributes += attributes;
    }

    m_rows = m_attributes.count();

    // force an update
    reset();
}
