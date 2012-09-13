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

#ifndef QFILEINFOEXT_H
#define QFILEINFOEXT_H

#include <QFileInfo>
#include <QUrl>

#include "qfileext.h"

#include "QFileExtensions_global.h"

//#define _VERBOSE_FILEINFOEXT 1

/**
  * This class extends the QFileInfo class to handle remote content information. When
  * instanciated, the remote content is cached by instanciating a QFileExt(remote_content).
  */

class QFILEEXTENSIONSSHARED_EXPORT QFileInfoExt : public QFileInfo {
public:
    explicit QFileInfoExt(const QString &url);
    virtual ~QFileInfoExt();

    bool isCopy() {
        return m_fileExtP && m_fileExtP->isCopy();
    }

    const QFileExt *getFileExt() {
        return m_fileExtP;
    }

    QString getUrl() {
        return m_fileExtP ? m_fileExtP->getUrl() : QUrl::fromLocalFile(absolutePath()).path();
    }

private:
    QFileExt *m_fileExtP;
};

#endif // QFILEINFOEXT_H
