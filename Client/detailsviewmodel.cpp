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

#include "detailsviewmodel.h"
#include "treenode.h"
#include "mainwindow.h"

DetailsViewModel::DetailsViewModel(MainWindow *windowP, QObject *parent) : QAbstractTableModel(parent) {
  m_windowP = windowP;
  m_numColumns = 0;
  m_numRows = 0;
}

/**
  * Returns the number of rows for the selected node. This is
  * the number of children in the 'selected filter'.
  */
int DetailsViewModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);

    return m_numRows;
}

/**
  * Returns the number of columns for the selected filter. This is
  * computed based on the number of attributes for the 'given 'selected'
  * filter's plugins.
  */
int DetailsViewModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);

    return m_numColumns;
}

/**
  * Returns the header data for the details content table.
  */
QVariant DetailsViewModel::headerData(int section, Qt::Orientation orientation, int role) const {
    QVariant result;

    // we don't do anything else than Display
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return result;

    // return the attribute name
    if (section < m_attributes.count())
        result.setValue(m_attributes[section]);

    return result;
}

/**
  * Returns the data for the attributes table.
  */
QVariant DetailsViewModel::data(const QModelIndex &index, int role) const {
    QVariant result;

#ifdef _VERBOSE_DETAILS_VIEW_MODEL
    qDebug() << "Details view model update(" << forceUpdate << ")";
#endif
    // we don't do anything else than Display
    if (role != Qt::DisplayRole &&
        role != Qt::DecorationRole &&
        role != Qt::UserRole)
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

    int nodeIndex = index.row();
    int attributeIndex = index.column();
    if (nodeIndex == -1 || attributeIndex == -1)
        return result;

    if (nodeIndex < m_nodeFilepaths.count()) {
        // standard attributes
        switch (role) {
            case Qt::DisplayRole:
                if (attributeIndex == 0)
                    result = m_nodes[nodeIndex];
                else
                    result = filterP->getAttributeValue(m_nodeFilepaths[nodeIndex], m_attributes[attributeIndex]);
#ifdef _VERBOSE_DETAILS_VIEW_MODEL
                qDebug() << "Details view model returns (display role) " << result << " at " << index.row() << ", " << index.column();
#endif
                break;

            case Qt::DecorationRole:
                if (attributeIndex == 0)
                    result.setValue(m_nodeIcons[nodeIndex]);
                break;

            case Qt::UserRole:
                result = m_nodePaths[nodeIndex];
#ifdef _VERBOSE_DETAILS_VIEW_MODEL
                qDebug() << "Details view model returns (user role) " << result << " at " << index.row() << ", " << index.column();
#endif
                break;
        }
    }

    return result;
}

/**
  * Forces an update of the attributes table.
  */
void DetailsViewModel::update(bool forceUpdate) {
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

    // we didn't move to a different filter, return the last values
    // unless the number of children changed or an update was requested
    if (filterP == m_lastSelectedFilterP &&
        filterP->childCount() == m_nodeFilepaths.count() &&
        !forceUpdate) {
        return;
    }

    // get the plugins
    m_attributes.clear();
    m_attributes << tr("Name");
    QStringList plugins = filterP->getPlugins();
    for (QStringList::iterator i = plugins.begin(); i != plugins.end(); i++)
        m_attributes += filterP->getPluginAttributes(*i);

    // compute number of rows/columns
    m_numRows = filterP->childCount();
    m_numColumns = m_attributes.count();

    // keep the ref of the last selected filter
    m_lastSelectedFilterP = filterP;

    // get the nodes' paths
    m_nodes.clear();
    m_nodePaths.clear();
    m_nodeIcons.clear();
    m_nodeFilepaths.clear();
    for (int i = 0; i < filterP->childCount(); i++) {
        TreeNode *nodeP = (TreeNode *)filterP->child(i);
        m_nodes += nodeP->text(NAME_HEADER_COLUMN);
        m_nodeFilepaths += nodeP->getPath();
        m_nodeIcons += nodeP->icon(NODE_ICON_COLUMN);
        m_nodePaths += nodeP->getNodePath();
    }

#ifdef _VERBOSE_DETAILS_VIEW_MODEL
    qDebug() << "Details view model has " << m_numRows << " rows and " << m_numColumns << " columns";
#endif

    // force an update
    reset();
}
