/*
 * SION! Server meta-data / javascript indexing server.
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

#include <QPluginLoader>
#include <QStringList>
#include <QThread>
#include <QDateTime>
#include <QtCore/QCoreApplication>
#include <QVariant>
#include <QDebug>

#include "filter.h"
#include "watcher.h"
#include "qfileinfoext.h"
#include "qdirext.h"

ServerDatabase Filter::m_db;

/**
  * The constructor initializes the filter object and loads the associated plugins.
  */

void Filter::loadPlugin(QString pluginFilename) {
    // load the plugin
    QPluginLoader loader(pluginFilename);
    if (loader.load()) {
#ifdef _VERBOSE_FILTER
        qDebug() << "Loaded plugin: " << pluginFilename;
#endif
        PluginInterface *pluginP = qobject_cast<PluginInterface *>(loader.instance());  // this singleton instance will be automatically
        pluginP = pluginP->newInstance(m_virtualDirectoryPath);                         // unloaded when the server exits
        m_plugins.append(pluginP);
        m_pluginFilenames.append(pluginFilename);
    }
    else
        qDebug() << QObject::tr("Failed to load plugin: ") + pluginFilename + QObject::tr(", error: ") + loader.errorString();
}

/**
  * Unloads and delete a plugin from this filter.
  */
void Filter::unloadPlugin(QString pluginFilename) {
    int index = m_pluginFilenames.indexOf(pluginFilename);
    m_pluginFilenames.removeAt(index);

    PluginInterface *pluginP = m_plugins[index];
    m_plugins.remove(index);

    delete pluginP;
}

/**
  * Constructs a filter under a parent filter, watching a physical directory (optionnally recursively)
  * using the given list of plugins.
  */
Filter::Filter(QString virtualDirectoryPath, QString url, bool recursive, QStringList pluginNames, Filter *parentP) {
    QFileInfoExt dirExt(url);

    // adds the filter to the db and keep its id
    m_db.addFilter(virtualDirectoryPath);
    m_filterId = m_db.getFilterId(virtualDirectoryPath);

    m_watcherP = NULL;

    // keep track of the physical hierarchy to later scan
    m_parentP = parentP;
    m_url = url;
    m_dir = dirExt.absoluteFilePath();
    m_recursive = recursive;

    // keep the virtual directory path
    m_virtualDirectoryPath = virtualDirectoryPath;

    // load the plugins (they reside in the current working directory)
    if (!pluginNames.isEmpty()) {
        for (int i = 0; i < pluginNames.count(); i++)
            loadPlugin(pluginNames[i]);

        // if at least one plugin was loaded, create the watcher for the given directory
        // (if existing)
        if (!parentP &&
            !m_plugins.isEmpty() &&
            !m_url.isEmpty() &&
            dirExt.exists())
            m_watcherP = new Watcher(m_url, recursive, this);
    }
}

/**
  * Deletes the filter. Doesn't delete the children filters (classifier handles
  * this).
  */
Filter::~Filter() {
#ifdef _VERBOSE_FILTER
    qDebug() << "deleting filter " << m_virtualDirectoryPath;
#endif

    displayActivity(tr("Deleting filter %1").arg(m_virtualDirectoryPath));

    if (m_watcherP)
        m_watcherP->acquireWatchSemaphore();

    if (m_watcherP) {
        if (isRunning())
            stop();
        delete m_watcherP;
    }

    // delete plugins
    qDeleteAll(m_plugins);
    m_plugins.clear();

    // deletes the filter from the db
    m_db.deleteFilter(m_filterId);
}

/**
  * Modifies the directory, recursivity or plugins of the filter.
  */
void Filter::modifyFilter(QString url, bool recursive, QStringList pluginFilenames) {
    QFileInfoExt dirExt(url);

    bool modified = false;
    bool watcherWasRunning = isRunning();

    if (isRunning())
        stop();

    // directory
    if (m_url != url) {
        m_url = url;
        m_dir = dirExt.absoluteFilePath();
        modified = true;
    }

    // recursivity
    if (m_recursive != recursive) {
        m_recursive = recursive;
        modified = true;
    }

    // plugins
    // unload the plugins we don't want anymore
    for (int i = m_pluginFilenames.count(); i > 0; i--) {
        QString pluginFilename = m_pluginFilenames[i - 1];

        if (pluginFilenames.contains(pluginFilename))
            continue; // keep this one

        unloadPlugin(pluginFilename);
        modified = true;
    }

    // load the new ones
    if (m_pluginFilenames != pluginFilenames) {
        if (!pluginFilenames.isEmpty()) {
            for (int i = 0; i < pluginFilenames.count(); i++) {
                QString pluginFilename = pluginFilenames[i];

                if (m_pluginFilenames.contains(pluginFilename))
                    continue; // keep this one, we already have it

                loadPlugin(pluginFilename);
                modified = true;
            }
        }
    }

    if (modified) {
        // do we have a watcher?
        if (m_watcherP) {
            delete m_watcherP;
            m_watcherP = NULL;
        }

        // if at least one plugin was loaded, create the watcher for the given directory
        // (if existing)
        if (!m_parentP &&
            !m_plugins.isEmpty() &&
            !m_url.isEmpty() &&
            dirExt.exists())
            m_watcherP = new Watcher(m_url, recursive, this);
    }

    if (watcherWasRunning)
        start();

    if (modified)
        rescanDirectory(); // force a refresh of the filtered files
}

/**
  * Checks the file referenced by the given absolute path against the plugins' rules and
  * optionaly save its reference into the db.
  */

bool Filter::checkAndSaveFile(QString path) {
    bool saved = false;

    // does the file rely under the watched directory?
    if (!path.startsWith(m_dir))
        return saved;


    // if any plugin accepts the file, then save its ref
    // in the db
    for (int i = 0; !saved && i < m_plugins.count(); i++) {
        m_plugins[i]->loadAttributes(path);
        saved |= m_plugins[i]->checkFile(path);
    }

    // save file if retained
    if (saved) {
        QString fileId = m_db.addFile(m_filterId, path); // add file to db

        // signal
        newFile(m_virtualDirectoryPath, path);

        // save file attributes
        for (int i = 0; i < m_plugins.count(); i++) {
            PluginInterface *fP = m_plugins[i];
            fP->loadAttributes(path); // attributes are loaded only if not done in the above checkFile iteration, we don't reload attrs if same file...
            QList<QString>attributes = fP->getAttributeNames();
            for (QList<QString>::iterator j = attributes.begin(); j != attributes.end(); j++) {
                    QString  attrName = (*j);
                    QVariant attrObjValue = fP->getAttributeValue(attrName);
                    QString attrValue = attrObjValue.isValid() ? attrObjValue.toString() : "<null>";
                    m_db.addFileAttribute(fileId, attrName, attrValue);
            }
        }
    }

    return saved;
}

/**
  * Matches (recursively) a new file against the filter rules (plugin' scripts). If the file is
  * filtered-in, it'll be saved in the DB.
  *
  * Edge Case: When the database is reloaded, the watcher is not in sync with the db, it hence
  * detects new files which are already in the db. This is the appropriate time to check whether
  * the file is still retained by the plugins since it could have been modified while the server
  * wasn't running or was running another filter set.
  */
void Filter::checkNewFile(QString path) {
    // Edge Case: if the file is already here (db was reloaded) check if it still matches the rules
    if (m_db.hasFile(m_filterId, path)) {
        checkModifiedFile(path);
        return;
    }

    // does the file rely under the watched directory?
    if (!path.startsWith(m_dir))
        return;

    // if no plugins, nothing to do
    if (m_plugins.isEmpty())
        return;

    // if a parent filter, the file must first have been
    // filtered in by the parent.
    if (m_parentP != NULL && !m_db.hasFile(m_parentP->m_filterId, path))
        return;

    // if any plugin accepts the file, then save its ref
    // in the db, then pass it over to the children
    if (checkAndSaveFile(path)) {
        // if children are present, broadcast check
        for (QVector<Filter *>::iterator i = m_children.begin(); i != m_children.end(); i++) {
            Filter *fP = (Filter *)(*i);
            if (fP)
                fP->checkNewFile(path);
        }
    }
}


/**
  * Matches (recursively) a modified file against the filter rules (plugin' scripts). If the file is
  * filtered-in, it'll be saved in the DB.
  */
void Filter::checkModifiedFile(QString path) {
    // does the file rely under the watched directory?
    if (!path.startsWith(m_dir))
        return;

    // if no plugins, nothing to do
    if (m_plugins.isEmpty())
        return;

    // if a parent filter, the file must first have been
    // filtered in by the parent, else, remove it from db.
    if (m_parentP != NULL && !m_db.hasFile(m_parentP->m_filterId, path)) {
        m_db.removeFile(m_filterId, path); // remove file from db

        // signal
        delFile(m_virtualDirectoryPath, path);
    }
    else {
        // if any plugin accepts the file, then save its ref
        // in the db. Else, if the file was in the db, remove it.
        if (!checkAndSaveFile(path) && m_db.hasFile(m_filterId, path)) {
            m_db.removeFile(m_filterId, path); // remove file from db

            // signal
            delFile(m_virtualDirectoryPath, path);
        }
    }

    // if children are present, broadcast check
    for (QVector<Filter *>::iterator i = m_children.begin(); i != m_children.end(); i++) {
        Filter *fP = (Filter *)(*i);
        fP->checkModifiedFile(path);
    }
}


/**
  * Removes any potentially retained deleted file from the db.
  */
void Filter::checkDeletedFile(QString path) {
    // does the file rely under the watched directory?
    if (!path.startsWith(m_dir))
        return;

    // if no plugins, nothing to do
    if (m_plugins.isEmpty())
        return;

    // just drop the file reference if it had previously been saved in the db
    if (m_db.hasFile(m_filterId, path)) {
        m_db.removeFile(m_filterId, path); // remove file from db

        // signal
        delFile(m_virtualDirectoryPath, path);

        // if children are present, broadcast check
        for (QVector<Filter *>::iterator i = m_children.begin(); i != m_children.end(); i++) {
            Filter *fP = (Filter *)(*i);
            fP->checkDeletedFile(path);
        }
    }
}


/**
 * Delete all children filters (called from destructor only). There's a redundant children deletion when the
 * classifier deletes a filter (since the filter's children are deleted, then the filter deleted, ...).
 *
 */
void Filter::deleteChildren() {
    qDeleteAll(m_children);
    m_children.clear();
}

/**
 * Delete all children filters and remove them from the passed list. We must protect this from
 * concurrency with the watcher thread raising a file/directory change to a... deleted filter.
 */
void Filter::deleteChildren(QVector<Filter *> *filtersP) {
    if (m_watcherP)
        m_watcherP->acquireWatchSemaphore();

    for (int i = m_children.count(); i > 0; i--) {
        Filter *fP = m_children[i - 1];
        fP->deleteChildren(filtersP);
        int fIndex = filtersP->indexOf(fP);
        if (fIndex != -1)
            filtersP->remove(fIndex);
        delete fP;
    }
    m_children.clear();

    if (m_watcherP)
        m_watcherP->releaseWatchSemaphore();

}

/**
 * Adds a child filter to this filter
 */
void Filter::addChild(Filter *childP) {
    if (!childP)
        return;

    if (m_watcherP)
        m_watcherP->acquireWatchSemaphore();

    m_children.append(childP);
    childP->setParent(this);

    if (m_watcherP)
        m_watcherP->releaseWatchSemaphore();
}

/**
 * Removes a child filter from this filter
 */
void Filter::removeChild(Filter *childP) {
    if (!childP)
        return;

    if (m_watcherP)
        m_watcherP->acquireWatchSemaphore();

    int index = m_children.indexOf(childP);
    if (index != -1)  {
        m_children.remove(index);
        childP->setParent(NULL);
    }

    if (m_watcherP)
        m_watcherP->releaseWatchSemaphore();
}

/**
 * Updates all associated filtered files in the db
 *
 */
void Filter::rescanDirectory() {
    cleanup();
    scanDirectory();
}

/**
 * Forces the filter to scan the associated directory. Shall be called *only* after the
 * Filter was created (and after its rules were set).
 */

void Filter::scanDirectory() {
    // delete root filter watcher and recreate one or
    // invoke parents up to root to do it.
    if (m_watcherP) {
        stop();
        delete m_watcherP;
        m_watcherP = new Watcher(m_dir, m_recursive, this);
        start();
    } else if (m_parentP)
        m_parentP->scanDirectory();
}

/**
 * Removes all associated filtered files from the db. This shall
 * be called *only* when a filter is removed.
 *
 */
void Filter::cleanup() {

    m_db.removeFiles(m_filterId); // remove all files from db

    // if children are present, broadcast cleanup
    for (QVector<Filter *>::iterator i = m_children.begin(); i != m_children.end(); i++) {
        Filter *fP = (Filter *)(*i);
        fP->cleanup();
    }
}

/**
 * Returns the loaded plugins
 */
QVector<PluginInterface *> *Filter::getPlugins() {
    return &m_plugins;
}

/**
  * Returns the javascript for the given plugin if found, an empty QString else.
  */
QString Filter::getScript(QString plugin) {
    for (QVector<PluginInterface *>::iterator i = m_plugins.begin(); i != m_plugins.end(); i++) {
        PluginInterface *fiP = *i;
        if (fiP->getName() == plugin) {
#ifdef _VERBOSE_FILTER
            qDebug() << "Filter::getScript(" << m_virtualDirectoryPath << ", " << plugin << ") -> " << fiP->getScript();
#endif
            return fiP->getScript();
        }
    }

    return "";
}

/**
  * Returns the attribute list for the given plugin if found, an empty list else.
  */
const QList<QString> Filter::getAttributes(QString plugin) {
    for (QVector<PluginInterface *>::iterator i = m_plugins.begin(); i != m_plugins.end(); i++) {
        PluginInterface *fiP = *i;
        if (fiP->getName() == plugin)
            return fiP->getAttributeNames();
    }

    return QList<QString>();
}

/**
  * Returns the attribute's class for the given plugin if found, an empty QString else.
  */
QString Filter::getAttributeClass(QString plugin, QString name) {
    for (QVector<PluginInterface *>::iterator i = m_plugins.begin(); i != m_plugins.end(); i++) {
        PluginInterface *fiP = *i;
        if (fiP->getName() == plugin)
            return fiP->getAttributeClassName(name);
    }

    return "";
}

/**
  * Return the attribute's tip for the given plugin if found, an empty QString else.
  */
QString Filter::getAttributeTip(QString plugin, QString name) {
    for (QVector<PluginInterface *>::iterator i = m_plugins.begin(); i != m_plugins.end(); i++) {
        PluginInterface *fiP = *i;
        if (fiP->getName() == plugin)
            return fiP->getAttributeTip(name);
    }

    return "";
}


/**
  * Returns the attribute's value for the given file 'path' if found, an empty QString else.
  */
QString Filter::getAttributeValue(QString path, QString name) {
    return m_db.getFileAttribute(m_filterId, path, name);
}

/**
  * Returns the tip for the given plugin if found, an empty QString else.
  */
QString Filter::getPluginTip(QString plugin) {
    for (QVector<PluginInterface *>::iterator i = m_plugins.begin(); i != m_plugins.end(); i++) {
        PluginInterface *fiP = *i;
        if (fiP->getName() == plugin)
            return fiP->getTip();
    }

    return "";
}

/**
  *Returns the javascript's last error for the given plugin if found, an empty QString else.
  */
QString Filter::getScriptLastError(QString plugin) {
    for (QVector<PluginInterface *>::iterator i = m_plugins.begin(); i != m_plugins.end(); i++) {
        PluginInterface *fiP = *i;
        if (fiP->getName() == plugin)
            return fiP->getScriptLastError();
    }

    return "";
}

/**
  * Sets the javascript for the given plugin if found, an empty QString else.
  */
void Filter::setScript(QString plugin, QString script) {
    for (QVector<PluginInterface *>::iterator i = m_plugins.begin(); i != m_plugins.end(); i++) {
        PluginInterface *fiP = *i;
        if (fiP->getName() == plugin) {
#ifdef _VERBOSE_FILTER
            qDebug() << "Filter::setScript(" << m_virtualDirectoryPath << ", " << plugin << ", " << script << ")";
#endif
            fiP->setScript(script);
            return;
        }
    }
}

/**
 * Start and stop methods for the watcher. Only the root filter has
 * a watcher.
 */
void Filter::stop() {
    // not root, propagate up
    if (m_parentP) {
        m_parentP->stop();
        return;
    }

    // stop watcher
    if (m_watcherP && m_watcherP->isRunning()) {
        m_watcherP->stop();

#ifdef _VERBOSE_FILTER
        qDebug() << "Filter is stopping and will block until watcher exits";
#endif

        m_watcherP->wait();

#ifdef _VERBOSE_FILTER
        qDebug() << "Filter unblocked";
#endif

        // so we don't get caught waiting for the semaphore
        // in case it wasn't release when the watcher was terminated
        m_watcherP->releaseWatchSemaphore();
    }
}

void Filter::start() {
    // not root, propagate up
    if (m_parentP) {
            m_parentP->start();
            return;
    }

    // start watcher
    if (m_watcherP && !m_watcherP->isRunning())
        m_watcherP->start();
}

/**
  * Returns (the filter or its parent's watcher is running)
  */
bool Filter::isRunning() {
    // not root, propagate up
    if (m_parentP)
        return m_parentP->isRunning();

    // ask watcher
    return m_watcherP && m_watcherP->isRunning();
}

/**
  * Returns the number of files under the associated physical directory.
  */
unsigned long Filter::numFiles(const QString &path) {
    unsigned long numFiles = 0;

    // count the directory files
    QDirExt dir(path);
    QStringList entries = dir.entryList();

    // walk through the directory entries
    for (QList<QString>::iterator i = entries.begin(); i != entries.end(); i++) {
        QString entryPath = path;
        entryPath.append(QDirExt::separator(path));
        entryPath.append(*i);
        QFileInfoExt entry(entryPath);

        if (entry.isHidden())
            continue;

        if (!entry.isDir())
            numFiles++;
    }

    return numFiles;
}

void Filter::directoryModified(const QString &path) {
#ifdef _VERBOSE_FILTER
    qDebug() << "Modified directory: " << path;
#else
    Q_UNUSED(path)
#endif
}

void Filter::directoryDeleted(const QString &path) {
#ifdef _VERBOSE_FILTER
    qDebug() << "Deleted directory: " << path;
#else
    Q_UNUSED(path)
#endif
}

void Filter::directoryAdded(const QString &path) {
#ifdef _VERBOSE_FILTER
    qDebug() << "New directory: " << path;
#else
    Q_UNUSED(path)
#endif
}

void Filter::fileModified(const QString &path) {
#ifdef _VERBOSE_FILTER
    qDebug() << "Modified file: " << path;
#endif

    checkModifiedFile(path);
}

void Filter::fileDeleted(const QString &path) {
#ifdef _VERBOSE_FILTER
    qDebug() << "Deleted file: " << path;
#endif

    checkDeletedFile(path);
}

void Filter::fileAdded(const QString &path) {
#ifdef _VERBOSE_FILTER
    qDebug() << "Added file: " << path;
#endif

    checkNewFile(path);
}
