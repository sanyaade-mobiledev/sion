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

#include "treenode.h"
#include "common.h"

/**
  * Constructs a TreeNode.
  */
TreeNode::TreeNode(QString name, QString path) : QTreeWidgetItem(QTreeWidgetItem::UserType) {
    m_path = path;
    setText(NAME_HEADER_COLUMN, name);
    setText(ENTRIES_HEADER_COLUMN, "0");
}

/**
  * Removes from the tree all the filters that are not present in the provided filters
  * list.
  */
void FilterTreeNode::removeFilters(QTreeWidgetItem *rootP, QStringList filters) {
    // if we have a filter, and this filter was not listed by the server, remove it.
    if (rootP->parent()) {
        TreeNode *nodeP = (TreeNode *)rootP;
        if (!nodeP->isFilter())
            return;

        QString filter = nodeP->getPath();
        if (!filter.isEmpty() && !filters.contains(filter)) {
            QTreeWidgetItem *parentP = nodeP->parent();
            if (parentP) {
                parentP->removeChild(nodeP);
                delete nodeP;
                return;
            }
        }
    }

    // get down the children filters..
    for (int i = rootP->childCount(); i > 0; i--)
        removeFilters(rootP->child(i - 1), filters);
}

/**
  * Remove all nodes under rootP.
  */
void FilterTreeNode::removeAll(TreeNode *rootP) {
    // get down into the children filters..
    for (int i = rootP->childCount(); i > 0; i--)
        removeAll((TreeNode *)rootP->child(i - 1));

    // remove this from its parent if not the root of the tree
    TreeNode *parentP = (TreeNode *)rootP->parent();
    if (parentP) {
        parentP->removeChild(rootP);
        delete rootP;
    }
}

/**
  * Returns the node which path under this is passed or NULL if not found.
  */
QTreeWidgetItem *FilterTreeNode::findNode(QString nodePath) {
    // split the path
    QStringList path = nodePath.split(VIRTUAL_PATH_SEPARATOR);

    // have we met the end of the path?
    if (path.count() == 1) {
        // check if this node has the expected child
        for (int i = 0; i < childCount(); i++) {
            QTreeWidgetItem *childP = child(i);
            if (childP->text(NAME_HEADER_COLUMN) == nodePath)
                return childP;
        }

        return NULL; // we didn't find the required node
    }

    // walk down the path

    // search the child which name matches the first path element
    for (int i = 0; i < childCount(); i++) {
        QTreeWidgetItem *childP = child(i);
        if (childP->text(NAME_HEADER_COLUMN) == path[0]) {
            nodePath.clear();
            for (QStringList::iterator j = ++path.begin(); j != path.end(); j++) {
                nodePath += *j;
                nodePath += VIRTUAL_PATH_SEPARATOR;
            }
            nodePath.chop(1);
            return ((TreeNode *)childP)->findNode(nodePath);
        }
    }

    return NULL;
}

/**
  * Empties a filter node (and its children filters if recursive).
  */
void FilterTreeNode::removeFiles(bool recursive) {
    // remove all files from filter node
    for (int i = childCount(); i > 0; i--) {
        TreeNode* childP = (TreeNode *)child(i - 1);
        if (!childP->isFilter()) {
            removeChild(childP);
            delete childP;
        } else if (recursive)
            childP->removeFiles(true);
    }
}

/**
  * If not already present, add a filter node below this at nodePath (path to the descendant node). Recursively
  * goes down the tree from this by walking through the path/tree.
  */
QTreeWidgetItem *FilterTreeNode::addFilterNode(QString fullPath, QString nodePath) {
    // split the path
    QStringList path = nodePath.split(VIRTUAL_PATH_SEPARATOR);

    // have we met the end of the path?
    if (path.count() == 1) {
        // check if this node doesn't already exist
        for (int i = 0; i < childCount(); i++) {
            QTreeWidgetItem *childP = child(i);
            if (((TreeNode *)childP)->isFilter() && childP->text(NAME_HEADER_COLUMN) == nodePath)
                return childP;
        }

#ifdef _VERBOSE_TREE
        qDebug() << "adding filter: " << nodePath;
#endif

        // this one doesn't exist, add it
        QTreeWidgetItem *childP = new FilterTreeNode(nodePath, fullPath);
        insertChild(0, childP); // filters initially appear before files in the children
        return childP;
    }

    // walk down the path

    // search the child which name matches the first path element
    for (int i = 0; i < childCount(); i++) {
        QTreeWidgetItem *childP = child(i);
        if (((TreeNode *)childP)->isFilter() && childP->text(NAME_HEADER_COLUMN) == path[0]) {
            nodePath.clear();
            for (QStringList::iterator j = ++path.begin(); j != path.end(); j++) {
                nodePath += *j;
                nodePath += VIRTUAL_PATH_SEPARATOR;
            }
            nodePath.chop(1);
            return ((TreeNode *)childP)->addFilterNode(fullPath, nodePath);
        }
    }

    return NULL;
}

/**
  * Returns the path from the root down to the given nodeP. The
  * returned path has the form: <root>"/"..."/"<nodeP>.
  */
QString TreeNode::getNodePath(QString &path) {
    TreeNode *parentP = (TreeNode *)parent();
    if (!parentP) {
        if (path.count()) // the root is a dummy node and shall not be included!
            path.remove(0, 1);

        return path;
    }

    path.prepend(text(NAME_HEADER_COLUMN));
    path.prepend(VIRTUAL_PATH_SEPARATOR);
    return parentP->getNodePath(path);
}
