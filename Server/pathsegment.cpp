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

#include <QDir>
#include <QDebug>

#include "pathsegment.h"

/**
  * Returns the full path of this PathSegment by walking recursively up to the root
  * PathSegment.
  */
QString PathSegment::getPath(QString &path) {
    if (!m_parentP || m_parentP->m_name.isEmpty()) {
        path.prepend(m_name);
        return path;
    }

    path.prepend(m_name);
    path.prepend(QDir::separator()); //  #### non local urls (<scheme>://<authority>/<path>) will be misformatted under windows, fix this
    return m_parentP->getPath(path);
}

/**
  * Returns the PathSegment corresponding to the given path if found
  * else NULL. Call from root.
  */
PathSegment *PathSegment::findPath(QStringList path) {
    PathSegment *segmentP = NULL;
    QString     segmentName;

    if (path.isEmpty())
        return segmentP;

    // keep the path segment name and remove it from the path
    segmentName = path[0];
    path.removeFirst();

   // in the case we have a "" in the path... (split can do this)
   if (segmentName.isEmpty())
        return findPath(path);

    // exit if this path doesn't exist
    segmentP = m_entries.value(segmentName);
    if (!segmentP)
        return segmentP;

    // have we met the end of the path?
    if (path.isEmpty())
        return segmentP;

    // walk down the path
    return segmentP->findPath(path);
}

/**
  * Adds a path to this PathSegment. Call from root.
  */
PathSegment *PathSegment::addPath(QStringList path, bool directory, QList<PathSegment *> *directoriesP, QList<PathSegment *> *filesP) {
    PathSegment *segmentP = NULL;
    QString     segmentName;

    if (path.isEmpty())
        return NULL;

    // keep the path segment name and remove it from the path
    segmentName = path[0];
    path.removeFirst();

    // in the case we have a "" in the path... (split can do this)
    if (segmentName.isEmpty())
        return addPath(path, directory, directoriesP, filesP);

    // check if this path segment doesn't already exist
    segmentP = m_entries.value(segmentName);
    if (!segmentP) {
        // this one doesn't exist, add it
        segmentP = new PathSegment(segmentName);
        segmentP->m_parentP = this;
        m_entries.insert(segmentName, segmentP);

        // add it to the path list
        if (path.isEmpty()) {
            if (directory)
                directoriesP->insert(0, segmentP);
            else
                filesP->insert(0, segmentP);
        }
    } else
        // we've got one more path going through this segment
        segmentP->m_refCount++;

    // have we met the end of the path?
    if (path.isEmpty())
        return segmentP;

    // walk down the path
    return segmentP->addPath(path, directory, directoriesP, filesP);
}

/**
  * Deletes a path from this PathSegment. Call from root. Returns true if the path
  * was found, false else.
  */
bool PathSegment::deletePath(QStringList path, bool directory, QList<PathSegment *> *directoriesP, QList<PathSegment *> *filesP) {
    PathSegment *segmentP = NULL;
    QString     segmentName;
    bool        deleted = false;

    if (path.isEmpty())
        return false;

    // keep the path segment name and remove it from the path
    segmentName = path[0];
    path.removeFirst();

   // in the case we have a "" in the path... (split can do this)
   if (segmentName.isEmpty())
        return deletePath(path, directory, directoriesP, filesP);

    // exit if this path doesn't exist
    segmentP = m_entries.value(segmentName);
    if (!segmentP)
        return false;

    // have we met the end of the path?
    if (path.isEmpty()) {
        if (segmentP && --segmentP->m_refCount == 0) {
            // this segment is not referenced, anymore, drop it
            m_entries.remove(segmentName);

            // remove it from the path list
            if (path.isEmpty()) {
                int index;
                if (directory) {
                    if ((index = directoriesP->indexOf(segmentP)) != -1)
                        directoriesP->removeAt(index);
                }
                else {
                    if ((index = filesP->indexOf(segmentP)) != -1)
                        filesP->removeAt(index);
                }
            }

            delete segmentP;

            return true;
        }

        return false;
    }

    // walk down the path
    if ((deleted = segmentP->deletePath(path, directory, directoriesP, filesP)) &&
        --segmentP->m_refCount == 0) {
        // this segment is not referenced, anymore, drop it
        m_entries.remove(segmentName);

        int index;
        if ((index = directoriesP->indexOf(segmentP)) != -1)
            directoriesP->removeAt(index);

        delete segmentP;

        return true;
    }

    return deleted;
}

/**
  * Recursively dumps this PathSegment.
  */
#ifdef _VERBOSE_PATH
void PathSegment::dump(QString message) {
    qDebug() << message << ", segment: " << m_name << ", ref count: " << m_refCount << ", nums entries: " << m_entries.count();
    QList<PathSegment *> list = m_entries.values();
    for (QList<PathSegment *>::iterator i = list.begin(); i != list.end(); i++)
        (*i)->dump("\t" + message);
}
#endif

/**
  * Merge the PathSet pointed by setP to this.
  */
PathSet *PathSet::merge(PathSet *setP) {
    if (!setP || setP->isEmpty())
        return this;

    // directories first
    for (QList<PathSegment *>::const_iterator i = setP->getPaths(true)->begin(); i != setP->getPaths(true)->end(); i++) {
#ifdef _VERBOSE_PATH
        qDebug() << "merging directory: " << (*i)->getPath();
#endif
        addPath((*i)->getPath(), true);
    }

    // then files
    for (QList<PathSegment *>::const_iterator i = setP->getPaths()->begin(); i != setP->getPaths()->end(); i++) {
#ifdef _VERBOSE_PATH
        qDebug() << "merging file: " << (*i)->getPath();
#endif
        addPath((*i)->getPath());
    }

    return this;
}

/**
  * Dumps this PathSet.
  */
#ifdef _VERBOSE_PATH
void PathSet::dump(QString message) {
    // num dirs
    qDebug() << "------------------- " << message << " -------------------";
    qDebug() << "\t num directories: " << m_directorySet.count();

    // directories first
    for (QList<PathSegment *>::const_iterator i = getPaths(true)->begin(); i != getPaths(true)->end(); i++)
        qDebug() << "\t directory: " << (*i)->getPath();

    // num files
    qDebug() << "\t num files: " << m_fileSet.count();

    // then files
    for (QList<PathSegment *>::const_iterator i = getPaths()->begin(); i != getPaths()->end(); i++)
        qDebug() << "\t file: " << (*i)->getPath();

    // now the tree
    qDebug() << "\t tree:";
    m_rootP->dump("\t\t");

    qDebug() << "\n\n";
}
#endif

/**
  * Delete all the paths of this PathSet.
  */
void PathSet::deleteAll() {
    // deleting root deletes everything
    delete m_rootP;

    // files and directories are just cleared (pointers to path segments were hold by the PathSegments tree.
    m_fileSet.clear();
    m_directorySet.clear();

    // recreate root
    m_rootP = new PathSegment("");
}
