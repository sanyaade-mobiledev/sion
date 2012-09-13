/*
 * SION! Client/Server remote content handling extensions.
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

#ifndef QDIREXT_H
#define QDIREXT_H

#include <QDir>
#include <QStringList>

#include "qfileinfoext.h"

#include "QFileExtensions_global.h"

//#define _VERBOSE_DIREXT 1

/**
  * This class extends the QDir class to handle remote directories. When
  * instanciated, the remote content is listed (not downloaded into the cache).
  */
class QFILEEXTENSIONSSHARED_EXPORT QDirExt : public QDir {
public:
    explicit QDirExt(const QString &url);

    const QStringList entryList(Filters filters = NoFilter, SortFlags sort = NoSort);

    static QChar separator(QString path);

private:
    QFileInfoExt    m_infoExt;  // create an extended file info for the directory
    QStringList     m_entries;

    void addEntries(QString root, const QString filename);
};

#endif // QDIREXT_H
