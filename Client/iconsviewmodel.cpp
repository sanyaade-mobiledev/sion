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

#include "iconsviewmodel.h"
#include "treenode.h"
#include "mainwindow.h"

/**
  * Constructs the icons view model.
  */
IconsViewModel::IconsViewModel(MainWindow *windowP, QObject *parent) : QAbstractTableModel(parent) {
  m_windowP = windowP;
  m_numColumns = 0;
  m_numRows = 0;
}

/**
  * Returns the number of rows for the selected node. This is
  * computed based on the number of items and the width of the table.
  * The computation is done in the update method.
  */
int IconsViewModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);

    return m_numRows;
}

/**
  * Returns the number of columns for the selected filter. This is
  * computed based on the number of items and the width of the table.
  * The computation is done in the update method.
  */
int IconsViewModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);

    return m_numColumns;
}

/**
  * Returns the data for the attributes table.
  */
QVariant IconsViewModel::data(const QModelIndex &index, int role) const {
    QVariant result;

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

    // compute the node index
    int row = index.row();
    int column = index.column();
    if (row == -1 || column == -1)
        return result;

    int nodeIndex = (row * m_numColumns) + column;
    if (nodeIndex < m_nodePaths.count()) {
        switch (role) {
            case Qt::DisplayRole:
                result = m_nodes[nodeIndex];
#ifdef _VERBOSE_ICONS_VIEW_MODEL
                qDebug() << "Icons view model returns (display role) " << result << " at " << index.row() << ", " << index.column();
#endif
                break;

            case Qt::DecorationRole:
                result.setValue(m_nodeIcons[nodeIndex]);
                break;

            case Qt::UserRole:
                result = m_nodePaths[nodeIndex];
#ifdef _VERBOSE_ICONS_VIEW_MODEL
                qDebug() << "Icons view model returns (user role) " << result << " at " << index.row() << ", " << index.column();
#endif
                break;
        }
    }

    return result;
}

/**
  * Forces an update of the attributes table.
  */
void IconsViewModel::update(QTableView *tableP, bool forceUpdate) {
#ifdef _VERBOSE_ICONS_VIEW_MODEL
    qDebug() << "Icons view model update(" << forceUpdate << ")";
#endif

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

    // compute number of rows/columns
    int numColumns = tableP->width() / CONTENT_COLUMN_WIDTH; // compute the number of columns based on the table width
    if (numColumns == 0)
        numColumns = 1;
    int numRows = (filterP->childCount() / numColumns) + 1;

    // does the layout change?
    bool layoutChanges = numColumns != m_numColumns;
    m_numColumns = numColumns;
    layoutChanges |= numRows != m_numRows;
    m_numRows = numRows;

    // we didn't move to a different filter, return the last values
    // unless the number of children changed or an update was requested
    if (filterP == m_lastSelectedFilterP &&
        filterP->childCount() == m_nodes.count() &&
        !layoutChanges &&
        !forceUpdate) {
        return;
    }

    // if only the layout changed, don't reload data, just force table reload/redraw
    if (filterP == m_lastSelectedFilterP &&
        filterP->childCount() == m_nodes.count() &&
        !forceUpdate) {
#ifdef _VERBOSE_ICONS_VIEW_MODEL
        qDebug() << "Icons view model Layout changed, has now " << m_numRows << " rows and " << m_numColumns << " columns";
#endif
        layoutChanged();
        return;
    }

    // there's something new to draw...

    // keep the ref of the last selected filter
    m_lastSelectedFilterP = filterP;

    // get the nodes' paths
    m_nodeIcons.clear();
    m_nodes.clear();
    m_nodePaths.clear();
    for (int i = 0; i < filterP->childCount(); i++) {
        TreeNode *nodeP = (TreeNode *)filterP->child(i);
        m_nodeIcons += nodeP->icon(NODE_ICON_COLUMN);
        m_nodePaths += nodeP->getNodePath();
        m_nodes += nodeP->text(NAME_HEADER_COLUMN);
    }

#ifdef _VERBOSE_ICONS_VIEW_MODEL
    qDebug() << "Icons view model has " << m_numRows << " rows and " << m_numColumns << " columns";
#endif

    // force an update
    reset();
}
