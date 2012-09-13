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

#ifndef PATHSEGMENT_H
#define PATHSEGMENT_H

#include <QMap>
#include <QStringList>
#include <QDir>
#include <QStack>
#include <QString>

//#define _VERBOSE_PATH 1

/**
  * A path segment is a segment of a file/directory path. If a directory,
  * a segment will store child segments in its m_entries QMap for fast searches.
  * The resulting tree represents a sets of files/directories organized as a tree,
  * hence saving space by sharing the common path segments (directories).
  */
class PathSegment {
public:
    PathSegment(QString name) {
        m_parentP = NULL;
        m_name = name;
        m_refCount = 1;
    }
    ~PathSegment() {
        qDeleteAll(m_entries);
        m_entries.clear();
    }

    static void sanitizePath(QString &path) {
        path.replace("://", "%SC%");
    }

    static void unsanitizePath(QString &path) {
        path.replace("%SC%", "://");
    }

    PathSegment *addPath(QStringList path, bool directory, QList<PathSegment *> *directoriesP, QList<PathSegment *> *filesP);
    bool        deletePath(QStringList path, bool directory, QList<PathSegment *> *directoriesP, QList<PathSegment *> *filesP);
    PathSegment *findPath(QStringList path);

    inline QMap<QString, PathSegment *> *entries() {
        return &m_entries;
    }

    inline bool isEmpty() {
        return m_entries.isEmpty();
    }

    inline QString getPath() {
        QString result;

        // retrieve the path, restore the scheme if any ("://")
        getPath(result);
        unsanitizePath(result);

        // local files must start with a '/'
        if (!result.contains("://"))
            result.prepend(QDir::separator());

        return result;
    }

    QString getPath(QString &path);

#ifdef _VERBOSE_PATH
    void dump(QString message = "dumping");
#endif

private:
    QString                         m_name;
    int                             m_refCount;   // number of path going through this
    QMap<QString, PathSegment *>    m_entries;
    PathSegment                     *m_parentP;
};

/**
  * A path set represents a set of files/directories stores as PathSegment trees.
  */
class PathSet {
public:
    PathSet() {
        m_rootP = new PathSegment("");
    }

    ~PathSet() {
        // deleting root deletes everything
        delete m_rootP;

        // files and directories are just cleared (pointers to path segments were held by the PathSegments tree).
        m_fileSet.clear();
        m_directorySet.clear();
    }

    void deleteAll();

    inline const PathSegment *getRoot() {
        return m_rootP;
    }

    inline bool isEmpty() {
        return m_rootP->isEmpty();
    }

    PathSet *merge(PathSet *setP);

    inline PathSegment *addPath(QString path, bool directory = false) {
        PathSegment::sanitizePath(path);
        QStringList segmentNames = path.split(QDir::separator());

        return m_rootP->addPath(segmentNames,
                                directory,
                                &m_directorySet,
                                &m_fileSet);
    }

    inline PathSegment *findPath(QString path) {
        PathSegment::sanitizePath(path);
        QStringList segmentNames = path.split(QDir::separator());

        return m_rootP->findPath(segmentNames);
    }

    inline bool deletePath(QString path, bool directory = false) {
        PathSegment::sanitizePath(path);
        PathSegment *segmentP;
        QStringList segmentNames = path.split(QDir::separator());

        if (!(segmentP = m_rootP->findPath(segmentNames)))
            return false;

        return m_rootP->deletePath(segmentNames,
                                   directory,
                                   &m_directorySet,
                                   &m_fileSet);
    }

#ifdef _VERBOSE_PATH
    void dump(QString message = "dumping");
#endif

    inline const QList<PathSegment *> *getPaths(bool directory = false) {
        return directory ? &m_directorySet : &m_fileSet;
    }

private:
    QList<PathSegment *>    m_fileSet;
    QList<PathSegment *>    m_directorySet;
    PathSegment             *m_rootP;
};

#endif // PATHSEGMENT_H
