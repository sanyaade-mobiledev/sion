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

#ifndef TREENODE_H
#define TREENODE_H

#include <QTreeWidgetItem>
#include <QFileIconProvider>

#include "qfileinfoext.h"

#define ENTRIES_HEADER_COLUMN 2

#define TYPE_HEADER_COLUMN    1
#define NODE_ICON_COLUMN      0
#define NAME_HEADER_COLUMN    0

#define FILE_TYPE             QObject::tr("File")
#define FILTER_TYPE           QObject::tr("Filter")

/**
  * The TreeNode class derives from the TreeWidgetItem class. The FilterNode
  * class holds a copy of the data maintained by the server. This copy is updated
  * periodically. The resulting tree serves as a model for the whole UI.
  */

// the 2 following classes are used to shadow the server info about the files/filters
class Attribute {
public:
    inline QString getClass() {
        return m_class;
    }
    inline void    setClass(QString className) {
        m_class = className;
    }

    inline QString getTip() {
        return m_tip;
    }
    inline void    setTip(QString tip) {
        m_tip = tip;
    }

private:
    QString m_class;
    QString m_tip;
};

class Plugin {
public:
    ~Plugin() {
        qDeleteAll(m_attributes);
        m_attributes.clear();
    }

    void setAttributes(QStringList attributes) {
        m_attributes.clear();

        // add a key and an empty attribute until we have more info about the attribute
        for (QStringList::iterator i = attributes.begin(); i != attributes.end(); i++) {
            QString attribute = *i;
            if (m_attributes.contains(attribute) || attribute.isEmpty())
                continue;

            m_attributes.insert(attribute, new Attribute());
        }
    }
    QStringList getAttributes() {
        return m_attributes.keys();
    }

    inline void setAttributeTip(QString attribute, QString tip) {
        Attribute *attributeP = findOrAddAttribute(attribute);
        if (attributeP)
            attributeP->setTip(tip);
    }

    inline QString getAttributeTip(QString attribute) {
        QString result;
        Attribute *attributeP = m_attributes[attribute];
        if (attributeP)
            result = attributeP->getTip();

        return result;
    }

    inline void setAttributeClass(QString attribute, QString className) {
        Attribute *attributeP = findOrAddAttribute(attribute);
        if (attributeP)
            attributeP->setClass(className);
    }

    inline QString getAttributeClass(QString attribute) {
        QString result;
        Attribute *attributeP = m_attributes[attribute];
        if (attributeP)
            result = attributeP->getClass();

        return result;
    }

    inline QString getScript() {
        return m_script;
    }
    inline void    setScript(QString script) {
        m_script = script;
    }
    inline QString getError() {
        return m_error;
    }
    inline void    setError(QString error) {
        m_error = error;
    }
    inline QString getTip() {
        return m_tip;
    }
    inline void    setTip(QString tip) {
        m_tip = tip;
    }

    inline QString getFilename() {
        return m_filename;
    }
    inline void setFilename(QString filename) {
        m_filename = filename;
    }

private:
    QMap<QString, Attribute *>m_attributes;
    QString                   m_script;
    QString                   m_error;
    QString                   m_tip;
    QString                   m_filename; // plugin lib name

    inline Attribute *findOrAddAttribute(QString attribute) {
        if (attribute.isEmpty())
            return NULL;

        Attribute *attributeP = m_attributes[attribute];
        if (!attributeP) {
            attributeP = new Attribute();
            m_attributes.insert(attribute, attributeP);
        }
        return attributeP;
    }
};

class TreeNode : public QTreeWidgetItem {
public:
    TreeNode(QString name, QString path);
    virtual ~TreeNode() {
    }

    inline void addChild(QTreeWidgetItem * child) {
        QTreeWidgetItem::addChild(child);
        updateEntries();
    }

    inline void addChildren (const QList<QTreeWidgetItem *> &children) {
        QTreeWidgetItem::addChildren(children);
        updateEntries();
    }

    inline void removeChild(QTreeWidgetItem * child) {
        QTreeWidgetItem::removeChild(child);
        updateEntries();
    }

    inline void insertChild(int index, QTreeWidgetItem * child) {
        QTreeWidgetItem::insertChild(index, child);
        updateEntries();
    }

    inline void insertChildren(int index, const QList<QTreeWidgetItem *> &children) {
        QTreeWidgetItem::insertChildren(index, children);
        updateEntries();
    }

    inline QTreeWidgetItem* takeChild(int index) {
        QTreeWidgetItem* childP = QTreeWidgetItem::takeChild(index);
        updateEntries();
        return childP;
    }

    inline QList<QTreeWidgetItem *> takeChildren() {
        QList<QTreeWidgetItem *> children = QTreeWidgetItem::takeChildren();
        updateEntries();
        return children;
    }

    inline virtual bool isShowFiles() {
        return false;
    }

    inline virtual void showFiles(bool /*show*/) {
    }

    inline virtual bool isFilter() {
        return false;
    }

    inline virtual QString getPath() {
        return m_path;
    }

    inline virtual void setPath(QString path) {
        m_path = path;
    }

    inline virtual void expandAllChildren() {
    }

    inline virtual void collapseAllChildren() {
    }

    inline virtual QTreeWidgetItem *findNode(QString nodePath) {
        Q_UNUSED(nodePath);
        return NULL;
    }

    inline virtual QTreeWidgetItem *addFilterNode(QString fullPath, QString nodePath) {
        Q_UNUSED(fullPath);
        Q_UNUSED(nodePath);
        return NULL;
    }

    inline virtual void removeFiles(bool recursive = false) {
        Q_UNUSED(recursive);
    }

    inline virtual void addNode(QString path) {
        Q_UNUSED(path);
    }

    inline virtual void deleteNode(QString path) {
        Q_UNUSED(path);
    }

    inline QString getNodePath() {
        QString path;
        return getNodePath(path);
    }

    QString getNodePath(QString &path);


    inline virtual QString getDirectory() {
        return "";
    }

    inline virtual void setDirectory(QString directory) {
        Q_UNUSED(directory);
    }


protected:
    QString m_path; // file or filter full path

private:
    inline void updateEntries() {
        QString entries;
        entries.sprintf("%d", childCount());
        setText(ENTRIES_HEADER_COLUMN, entries);
    }
};

class FileTreeNode : public TreeNode {
public:
    explicit FileTreeNode(QIcon *iconP, QString name, QString path) : TreeNode(name, path) {
        setText(TYPE_HEADER_COLUMN, FILE_TYPE);
        setIcon(NODE_ICON_COLUMN, *iconP);
    }

    virtual QString getDirectory() {
        QFileInfoExt fileInfo(m_path);
        return fileInfo.absolutePath();
    }

    inline void setAttributeValue(QString attribute, QString value) {
        m_attributeValues.insert(attribute, value);
    }

    inline QString getAttributeValue(QString attribute) {
        return m_attributeValues[attribute];
    }

private:
    QMap<QString, QString> m_attributeValues;
};

class FilterTreeNode : public TreeNode {
public:
    FilterTreeNode(QString name, QString path) : TreeNode(name, path) {
        m_showFiles = false; // filters don't show their files by default
        setText(TYPE_HEADER_COLUMN, FILTER_TYPE);
        setIcon(NODE_ICON_COLUMN, QIcon(":/icon/FilterFolder"));
    }
    ~FilterTreeNode() {
        qDeleteAll(m_plugins);
        m_plugins.clear();
    }

    inline bool isShowFiles() {
        return m_showFiles;
    }

    inline void showFiles(bool show) {
        m_showFiles = show;
    }

    inline bool isFilter() {
        return true;
    }

    inline bool fileDataLoaded(FileTreeNode *fileP) {
        QStringList plugins = getPlugins();
        if (plugins.isEmpty())
            return true;
        QString lastPlugin = plugins[plugins.count() - 1];
        QStringList attributes = getPluginAttributes(lastPlugin);
        if (attributes.isEmpty())
            return true;
        QString lastAttribute = attributes[attributes.count() - 1];
        return fileP && !fileP->getAttributeValue(lastAttribute).isEmpty();
    }

    void expandAllChildren() {
        for (int i = 0; i < childCount(); i++) {
            TreeNode *childP = (TreeNode *)child(i);
            childP->expandAllChildren();
            childP->setExpanded(true);
        }
    }

    void collapseAllChildren() {
        for (int i = 0; i < childCount(); i++) {
            TreeNode *childP = (TreeNode *)child(i);
            childP->collapseAllChildren();
            childP->setExpanded(false);
        }
    }

    QTreeWidgetItem *findNode(QString nodePath);

    static void removeFilters(QTreeWidgetItem *rootP, QStringList filters);
    static void removeAll(TreeNode *rootP);

    QTreeWidgetItem *addFilterNode(QString fullPath, QString nodePath);

    void removeFiles(bool recursive = false);

    void addNode(QString path) {
        // if the filter doesn't show files
        // don't add, because the files will be requested again
        // when the filter will be asked to show the files...
        if (!m_showFiles)
            return;

        // check if not already there
        for (int i = 0; i < childCount(); i++) {
            TreeNode *childP = (TreeNode *)child(i);
            if (childP->getPath() == path)
                return;
        }

        // add the new node
        QFileInfoExt fileInfo(path);
        QFileIconProvider ip;
        QIcon icon = ip.icon(fileInfo);
        addChild(new FileTreeNode(&icon, fileInfo.fileName(), path));
    }

    void deleteNode(QString path) {
        bool found = false;
        TreeNode *nodeP;
        for (int i = 0; !found && i < childCount(); i++) {
            nodeP = (TreeNode *)child(i);
            if (nodeP->getPath() == path)
                found = true;
        }

        if (found) {
            removeChild(nodeP);
            delete nodeP;
        }
    }

    inline QString getDirectory() {
        return m_directory;
    }

    inline void setDirectory(QString directory) {
        m_directory = directory;
    }

    void setPlugins(QStringList plugins) {
        qDeleteAll(m_plugins);
        m_plugins.clear();

        // empty plugin list or misformed plugin list
        if (plugins.count() & 1)
            return;

        // the first item is the plugin filename and the second is its human
        // readable name.
        for (QStringList::Iterator i = plugins.begin(); i != plugins.end();) {
            QString filename = *i++;
            QString pluginName = *i++;
            Plugin *pluginP = new Plugin();
            pluginP->setFilename(filename);
            m_plugins.insert(pluginName, pluginP);
        }
    }

    inline QStringList getPlugins() {
        return m_plugins.keys();
    }

    inline QStringList getPluginFilenames() {
        QStringList filenames;
        for (QMap<QString, Plugin *>::iterator i = m_plugins.begin(); i != m_plugins.end(); i++) {
            Plugin *pluginP = *i;
            filenames += pluginP->getFilename();
        }

        return filenames;
    }

    inline void setPluginTip(QString plugin, QString tip) {
        findOrAddPlugin(plugin)->setTip(tip);
    }

    inline QString getPluginTip(QString plugin) {
        QString result;
        Plugin *pluginP = m_plugins[plugin];
        if (pluginP)
            result = pluginP->getTip();

        return result;
    }

    inline void setPluginScript(QString plugin, QString script) {
        findOrAddPlugin(plugin)->setScript(script);
    }

    inline QString getPluginScript(QString plugin) {
        QString result;
        Plugin *pluginP = m_plugins[plugin];
        if (pluginP)
            result = pluginP->getScript();

        return result;
    }

    inline void setPluginScriptError(QString plugin, QString error) {
        findOrAddPlugin(plugin)->setError(error);
    }

    inline QString getPluginScriptError(QString plugin) {
        QString result;
        Plugin *pluginP = m_plugins[plugin];
        if (pluginP)
            result = pluginP->getError();

        return result;
    }

    inline void setPluginAttributes(QString plugin, QStringList attributes) {
        Plugin *pluginP = findOrAddPlugin(plugin);
        pluginP->setAttributes(attributes);
    }

    inline QStringList getPluginAttributes(QString plugin) {
        QStringList result;
        Plugin *pluginP = m_plugins[plugin];
        if (pluginP)
            result = pluginP->getAttributes();

        // #### where does this empty attribute come from? fix this
        while (result.removeOne(""));

        return result;
    }

    inline void setPluginAttributeClass(QString plugin, QString attribute, QString className) {
        Plugin *pluginP = findOrAddPlugin(plugin);
        pluginP->setAttributeClass(attribute, className);
    }

    inline QString getAttributeClass(QString plugin, QString attribute) {
        QString result;
        Plugin *pluginP = m_plugins[plugin];
        if (pluginP)
            result = pluginP->getAttributeClass(attribute);

        return result;
    }

    inline void setPluginAttributeTip(QString plugin, QString attribute, QString tip) {
        Plugin *pluginP = findOrAddPlugin(plugin);
        pluginP->setAttributeTip(attribute, tip);
    }

    inline QString getAttributeTip(QString plugin, QString attribute) {
        QString result;
        Plugin *pluginP = m_plugins[plugin];
        if (pluginP)
            result = pluginP->getAttributeTip(attribute);

        return result;
    }

    QString getAttributeValue(QString path, QString attribute) {
        QString result;

        // try to find the file
        TreeNode *nodeP = NULL;
        for (int i = 0; !nodeP && i < childCount(); i++) {
            if (((TreeNode *)child(i))->getPath() == path)
                nodeP = (TreeNode *)child(i);
        }

        if (nodeP && !nodeP->isFilter())
            result = ((FileTreeNode *)nodeP)->getAttributeValue(attribute);

        return result;
    }

    void setAttributeValue(QString path, QString attribute, QString value) {
        // try to find the file
        TreeNode *nodeP = NULL;
        for (int i = 0; !nodeP && i < childCount(); i++) {
            if (((TreeNode *)child(i))->getPath() == path)
                nodeP = (TreeNode *)child(i);
        }

        if (nodeP && !nodeP->isFilter())
            ((FileTreeNode *)nodeP)->setAttributeValue(attribute, value);
    }

    void setRunning(bool running) {
        m_running = running;
    }

    bool isRunning() {
        return m_running;
    }

private:
    bool                    m_showFiles; // if a filter, contained files must be shown.
    bool                    m_running;   // filter is running.
    QString                 m_directory;
    QMap<QString, Plugin *> m_plugins;   // plugin name is the key

    inline Plugin *findOrAddPlugin(QString plugin) {
        Plugin *pluginP = m_plugins[plugin];
        if (!pluginP) {
            pluginP = new Plugin();
            m_plugins.insert(plugin, pluginP);
        }
        return pluginP;
    }
};

#endif // TREENODE_H
