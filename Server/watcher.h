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

#ifndef WATCHER_H
#define WATCHER_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QDateTime>
#include <QDebug>

#include "filter.h"
#include "pathsegment.h"

//#define _VERBOSE_WATCHER 1

#define WATCH_PASS_INTERVAL             5000 // poll the directories every WATCH_INTERVAL millisecs
#define NEW_DIRS_PER_PASS               2    // don't scan more than this per pass (at the top level, so this is already exponential...)

#define MAX_WATCHED_FILES               5000 // max files watched per watcher

/**
  * The watcher embeds a thread to keep track of the associated directory/ies changes.
  * It signals when a change occured in the watched objects. It can be started/stopped when required.
  */

class Watcher : public QThread {
    Q_OBJECT

public:
    explicit Watcher(QString url, bool recursive, Filter *filterP);

    inline void acquireWatchSemaphore() {
        m_watchSem.acquire();
    }

    inline bool tryAcquireWatchSemaphore() {
        return m_watchSem.tryAcquire();
    }

    void releaseWatchSemaphore() {
        while (!m_watchSem.available())
            m_watchSem.release();
    }

    inline void stop() {
#ifdef _VERBOSE_WATCHER
        qDebug() << "WATCHER STOP!";
#endif

        // disconnect the scan results signals/slots
        disconnect(m_filterP, SLOT(fileAdded(QString)));
        disconnect(m_filterP, SLOT(fileDeleted(QString)));
        disconnect(m_filterP, SLOT(fileModified(QString)));
        disconnect(m_filterP, SLOT(directoryAdded(QString)));
        disconnect(m_filterP, SLOT(directoryDeleted(QString)));
        disconnect(m_filterP, SLOT(directoryModified(QString)));

        m_stop = true;
    }

    inline void start() {
#ifdef _VERBOSE_WATCHER
        qDebug() << "WATCHER START!";
#endif
        m_stop = false;
        QThread::start(IdlePriority);
    }

signals:
    void displayActivity(QString);
    void displayProgress(int min, int max, int value);

    void fileAdded(const QString &path);
    void fileDeleted(const QString &path);
    void fileModified(const QString &path);
    void directoryAdded(const QString &path);
    void directoryDeleted(const QString &path);
    void directoryModified(const QString &path);

protected:
    void run();

private:
    Filter          *m_filterP;
    volatile bool   m_stop;         // stop running...
    QSemaphore      m_watchSem;     // to prevent concurrent access with the filter to resources while scanning
    QString         m_url;          // root url we watch
    PathSet         m_files;        // the watched files (including directories)
    PathSet         m_newFiles;     // the new watched files (including directories)
    PathSet         m_removedFiles; // the deleted watched files (including directories)
    int             m_numWatchedFiles; // number of files watched so far
    bool            m_recursive;       // recursively go down directories
    QDateTime       m_lastPass;     // last pass time

    void watchDirectory(QString directory);

    void getNewSubDirectories(QString dir);
};

#endif // WATCHER_H
