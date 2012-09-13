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

#include <QApplication>

#include "watcher.h"
#include "qdirext.h"
#include "qfileinfoext.h"

/**
  * Creates a new instance of watcher thread. The watcher is not started but keeps the references
  * to the directory to be watched, and the associated filter. The lastPass member is used to
  * detect modification/creation of files between two passes.
  */
Watcher::Watcher(QString url, bool recursive, Filter *filterP) : QThread(), m_watchSem(1) {
    m_url = url;
    m_recursive = recursive;
    m_filterP = filterP;
    m_lastPass = QDateTime::currentDateTime();

    // connect the activity signals/slots
    connect(this, SIGNAL(displayActivity(QString)), filterP, SIGNAL(displayActivity(QString)));
    connect(this, SIGNAL(displayProgress(int,int,int)), filterP, SIGNAL(displayProgress(int,int,int)));

    // connect the scan results signals/slots
    connect(this, SIGNAL(fileAdded(QString)), filterP, SLOT(fileAdded(QString)));
    connect(this, SIGNAL(fileDeleted(QString)), filterP, SLOT(fileDeleted(QString)));
    connect(this, SIGNAL(fileModified(QString)), filterP, SLOT(fileModified(QString)));
    connect(this, SIGNAL(directoryAdded(QString)), filterP, SLOT(directoryAdded(QString)));
    connect(this, SIGNAL(directoryDeleted(QString)), filterP, SLOT(directoryDeleted(QString)));
    connect(this, SIGNAL(directoryModified(QString)), filterP, SLOT(directoryModified(QString)));
}

/**
  * Returns in m_newFiles the newly found sub directories of root.
  */
void Watcher::getNewSubDirectories(QString root) {
    if (root.isEmpty() || m_stop)
        return;

    //  add new root to the list of directories
    m_newFiles.addPath(root, true);

    // signal new directory
    directoryAdded(root);

    // create a directory
    QDirExt rootDir(root);

    // get the directory entries
    QStringList entries = rootDir.entryList();

    if (entries.isEmpty())
        return;

    // give the user a hint about what's going on down here
    displayActivity(tr("Exploring directory %1").arg(root));

    // walk down recursively through the directories
    for (QList<QString>::iterator i = entries.begin(); !m_stop && i != entries.end(); i++) {
        QString entryPath = root;
        entryPath.append(QDirExt::separator(root));
        entryPath.append(*i);
        QFileInfoExt entry(entryPath);

        if (entry.isHidden())
            continue;

        if (entry.isDir()) {
            // recursively go through children dirs if required
            if (m_recursive && !m_files.findPath(entryPath)) {
                getNewSubDirectories(entryPath);

#ifdef _VERBOSE_WATCHER
                qDebug() << "Detected new directory " << entryPath;
#endif
            }
        }
    }
}

/**
  * Watches the given directory. Detects the new directories but delegates their initial inspection
  * to the getSubDirectories method. The latter will recursively list all of their sub directories.
  */
void Watcher::watchDirectory(QString directory) {
    if (m_stop)
        return;

    // now watch all contained files
    QDirExt rootDir(directory);

    // get the directory entries
    QStringList entries = rootDir.entryList();

#ifdef _VERBOSE_WATCHER
    qDebug() << "Watching into " << directory;
#endif

    // has the directory been removed?
    // if so, just return, the removal signal will be sent later on.
    QFileInfoExt entryInfo(directory);
    if (!entryInfo.exists())
        return;

    // has the directory changed since last pass?
    if (entryInfo.lastModified() >= m_lastPass) {
#ifdef _VERBOSE_WATCHER
        qDebug() << "Detected modified directory " << directory;
#endif
        // signal
        directoryModified(directory);
    }

    // walk through the list
    int numNewDirs = 0;
    for (QList<QString>::iterator i = entries.begin(); !m_stop && numNewDirs < NEW_DIRS_PER_PASS && i != entries.end(); i++) {
        QString entryPath = directory;
        entryPath.append(QDirExt::separator(directory));
        entryPath.append(*i);
        QFileInfoExt entryInfo(entryPath);

        if (entryInfo.isHidden())
            continue; // we don't care about these ones.

        if (!entryInfo.isDir()) {
            // a regular file found

            // is it new?
            if (!m_files.findPath(entryPath) && m_numWatchedFiles < MAX_WATCHED_FILES) {
                m_newFiles.addPath(entryPath);
                ++m_numWatchedFiles;

#ifdef _VERBOSE_PATH
            m_newFiles.dump("file added, dumping new files");
#endif

#ifdef _VERBOSE_WATCHER
            qDebug() << "Detected new file " << entryPath;
#endif
                // signal new file
//              fileAdded(entryPath);
                fileAdded(entryInfo.absoluteFilePath());
            } else if (entryInfo.lastModified() >= m_lastPass) {
#ifdef _VERBOSE_WATCHER
                qDebug() << "Detected modified file " << entryPath;
#endif
                // signal modified file
//              fileModified(entryPath);
                fileModified(entryInfo.absoluteFilePath());
            }
        } else {
            // if the directory is not in the list, and doing a recursive watch, browse it.
            if (m_recursive && !m_files.findPath(entryPath)) {
                // go through new dir's content
                displayProgress(0, 0, 0); // back and forth moving progress
                getNewSubDirectories(entryPath);
#ifdef _VERBOSE_PATH
                m_newFiles.dump("new directory browsed, dumping new files");
#endif
                ++numNewDirs;
            }
        }
    }
}

/**
  * This is the watcher thread main stuff. It iterates on watching its 'known' directories and
  * notifying the associated filter with the directories/files changes.
  */
void Watcher::run() {
    QTime                       passStart;
    int                         numEntries = 0;
    const QList<PathSegment *>  *pathsP = NULL;

    // do nothing if no root url set
    if (m_url.isEmpty())
        return;

    m_stop = false;

    // we'll start by scanning this one
    m_files.addPath(m_url, true);

    do {
        passStart.restart();

#ifdef _VERBOSE_WATCHER
        qDebug() << "(File)Watcher does a pass at " << passStart;
#endif
        // now, don't let the filter play concurrently
        if (!tryAcquireWatchSemaphore())
            goto nextPass;

        // check for new or modified files/directories
        pathsP = m_files.getPaths(true);
        numEntries = pathsP->count();
        for (int i = 0; !m_stop && i < numEntries; i++) {
            QString filepath = pathsP->at(i)->getPath();

            // show progress
            displayActivity(tr("Scanning directory %1").arg(filepath));
            displayProgress(0, numEntries - 1, i);

            watchDirectory(filepath);
        }

        // add the new files and directories to the watch lists
#ifdef _VERBOSE_PATH
        m_files.dump("dumping watched files before merging");
        m_newFiles.dump("dumping new files before merging");
#endif

        m_files.merge(&m_newFiles);
        m_newFiles.deleteAll();

#ifdef _VERBOSE_PATH
        m_files.dump("dumping watched files after merging");
        m_newFiles.dump("dumping new files after merging");
#endif

        // now check if any of the watched files have disappeared

        // check removed directories
        for (QList<PathSegment *>::const_iterator i = m_files.getPaths(true)->begin(); i != m_files.getPaths(true)->end(); i++) {
            QString filepath = (*i)->getPath();
            QFileInfoExt entryInfo(filepath);
            if (!entryInfo.exists()) {
                // we DO NOT need to check the content and signal for everything...
                // because the content will be checked later on...
#ifdef _VERBOSE_WATCHER
                qDebug() << "Detected deleted directory " << filepath;
#endif
                directoryDeleted(filepath);
                m_removedFiles.addPath(filepath, true);
            }
        }

        // check removed files
        for (QList<PathSegment *>::const_iterator i = m_files.getPaths()->begin(); !m_stop && i != m_files.getPaths()->end(); i++) {
            QString filepath = (*i)->getPath();
            QFileInfoExt entryInfo(filepath);
            if (!entryInfo.exists()) {
#ifdef _VERBOSE_WATCHER
                qDebug() << "Detected deleted file " << filepath;
#endif
//              fileDeleted(filepath);
                fileDeleted(entryInfo.absoluteFilePath());
                m_removedFiles.addPath(filepath);
                --m_numWatchedFiles;
            }
        }

        // and remove

        // directories
        for (QList<PathSegment *>::const_iterator i = m_removedFiles.getPaths(true)->begin(); i != m_removedFiles.getPaths(true)->end(); i++)
            m_files.deletePath((*i)->getPath(), true);

        // files
        for (QList<PathSegment *>::const_iterator i = m_removedFiles.getPaths()->begin(); !m_stop && i != m_removedFiles.getPaths()->end(); i++)
            m_files.deletePath((*i)->getPath());

        m_removedFiles.deleteAll();

        // keep last pass time
        m_lastPass = QDateTime::currentDateTime();

        // now, the filter can play
        releaseWatchSemaphore();

nextPass:
#ifdef _VERBOSE_WATCHER
        qDebug() << "(File)Watcher ends pass at " << m_lastPass << " (duration: " << passStart.elapsed() << " ms)";
#endif
        QString duration;
        duration.sprintf("%d", passStart.elapsed());
        displayActivity(tr("Scanning pass completed in (%1) milliseconds. Now sleeping.").arg(duration));
        displayProgress(1, 100, 100);

        // a little break
        if (!m_stop)
            msleep(WATCH_PASS_INTERVAL);
    } while (!m_stop);

#ifdef _VERBOSE_WATCHER
    qDebug() << "(File)Watcher now EXITING!";
#endif
}
