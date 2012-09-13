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

#include <QUrl>
#include <QDebug>

#include "qfileinfoext.h"


/**
  * Retrieves information on the content referred by the given url.
  */
QFileInfoExt::QFileInfoExt(const QString &url) : QFileInfo() {
#ifdef _VERBOSE_FILEINFOEXT
    qDebug() << "instanciating QFileInfoExt(" << url << ")";
#endif

    m_fileExtP = NULL;

    // does the url point to a file?
    QUrl    fullUrl(url);
    QString urlPath = fullUrl.path();
    QString scheme = fullUrl.scheme();

    if (scheme.isEmpty() || scheme == "file") {
#ifdef _VERBOSE_FILEINFOEXT
        qDebug() << "QFileInfoExt refers to a local file";
#endif
        // just initializes this as a normal file
        setFile(urlPath);
    } else if (scheme == "http") {
#ifdef _VERBOSE_FILEINFOEXT
        qDebug() << "QFileInfoExt refers to a remote content";
#endif
        // create a local copy of the remote content and
        // sets this copy as the file
        m_fileExtP = new QFileExt(url);
        setFile(m_fileExtP->fileName());
    }
}

/**
  * Deletes the associated cached content file.
  */
QFileInfoExt::~QFileInfoExt() {
#ifdef _VERBOSE_FILEINFOEXT
    qDebug() << "QFileInfoExt(" << this->fileName() << ") destructor invoked";
#endif

    // delete the local copy of the remote content if any
    if (m_fileExtP)
        delete m_fileExtP;
}
