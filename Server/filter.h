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

#ifndef FILTER_H
#define FILTER_H

#include <QThread>
#include <QVector>
#include <QSemaphore>
#include <QStringList>

#include "filter.h"
#include "plugininterface.h"
#include "serverdatabase.h"

//#define _VERBOSE_FILTER 1

/**
 * A Filter uses instances of plugins and watchers to watch the directory's content.
 * The watcher, usually associated with the root filter, is responsible for scanning
 * the files in the filter's directory.
 *
 * A filter is associated to a virtual directory path in the browser and
 * applies to all files in the physical directory and its subdirectories
 * if recursive.
 *
 * A filter can have children. In this case, children filters apply to the
 * files filtered in by the parent filter, not the physical files. Thus, children
 * filters do not watch directly a directory unless explicitly asked for a 'rescan'.
 *
 * A parent filter is responsible for broadcasting the check<Created/Modified/Deleted>File
 * method invocation to its children.
 *
 * When deleting a filter, all of the children filters are deleted (and so on, recursively).
 *
 * IMPORTANT: the virtualDirectoryPath passed when creating a filter is the fully qualified
 * path starting from the topmost filter parent down to the newly created filter (included)
 */

class Watcher;
class Filter : public QObject {
    Q_OBJECT

signals:
    void newFile(QString virtualDirectoryPath, QString path);
    void delFile(QString virtualDirectoryPath, QString path);
    void displayActivity(QString string);
    void displayProgress(int min, int max, int value);

public slots:
    void fileAdded(const QString &path);
    void fileDeleted(const QString &path);
    void fileModified(const QString &path);
    void directoryAdded(const QString &path);
    void directoryDeleted(const QString &path);
    void directoryModified(const QString &path);

public:
    explicit Filter(QString virtualDirectoryPath, QString url, bool recursive, QStringList pluginNames, Filter *parentP = 0);
    ~Filter();

    void modifyFilter(QString url, bool recursive, QStringList pluginFilenames);

    void deleteChildren(QVector<Filter *> *filtersP);

    void addChild(Filter *childP);
    void removeChild(Filter *childP);

    void scanDirectory();
    void rescanDirectory();

    void cleanup();

    QVector<PluginInterface *> *getPlugins();
    QString getPluginTip(QString plugin);
    QString getScript(QString plugin);
    QString getScriptLastError(QString plugin);
    void    setScript(QString plugin, QString script);

    const QList<QString> getAttributes(QString plugin);
    QString getAttributeClass(QString plugin, QString name);
    QString getAttributeTip(QString plugin, QString name);
    QString getAttributeValue(QString path, QString name);

    inline QString getDirectory() {
        return m_dir;
    }

    inline QString getUrl() {
        return m_url;
    }

    inline QString getFilterId() {
        return m_filterId;
    }

    inline QString getVirtualDirectoryPath() {
        return m_virtualDirectoryPath;
    }

    inline bool isRecursive() {
        return m_recursive;
    }

    inline Filter *getParent() {
        return m_parentP;
    }

    inline QStringList getPluginFilenames() {
        return m_pluginFilenames;
    }

    void stop();    // call these ones to start/stop a whole filter tree.
    void start();
    bool isRunning();

    inline bool isRoot() {
        return !m_parentP;
    }

    unsigned long numFiles(const QString &path);

private:
    Filter                          *m_parentP;     // parent filter (if any)
    QVector<Filter *>               m_children;     // children filters
    QString                         m_dir;          // physical dir to watch (can a local copy of the content referred by m_url)
    QString                         m_url;          // url to watch
    bool                            m_recursive;    // whether we recursively watch through the sub-directories starting from dir
    Watcher                         *m_watcherP;
    QVector<PluginInterface *>      m_plugins;      // WARNING: these two sets MUST contain the plugin in the same order
    QStringList                     m_pluginFilenames;
    QString                         m_virtualDirectoryPath;
    QString                         m_filterId;     // computed and help in the db
    static ServerDatabase           m_db;

    inline void setParent(Filter *parentP) {
        m_parentP = parentP;
    }

    void checkNewFile(QString path);
    void checkModifiedFile(QString path);
    void checkDeletedFile(QString path);

    bool checkAndSaveFile(QString path);

    void deleteChildren();

    void loadPlugin(QString pluginName);
    void unloadPlugin(QString pluginFilename);
};

#endif // FILTER_H

