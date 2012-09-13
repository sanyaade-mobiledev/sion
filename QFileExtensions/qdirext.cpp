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

#include <QDebug>

#include "qdirext.h"
#include "qfileinfoext.h"

/**
  * Builds a local/remote content directory based on the given url. Retrieves the root
  * url. If the root content is remote, downloads it, then lists its content. So far, only remote
  * http scheme and html root content is supported.
  */
QDirExt::QDirExt(const QString &url) : QDir(), m_infoExt(url) {
#ifdef _VERBOSE_DIREXT
    qDebug() << "instanciating QDirExt(" << url << ")";
#endif

    // sets this dir' path
    QString root = m_infoExt.isDir() ? m_infoExt.absoluteFilePath() : m_infoExt.absolutePath();
    setPath(root);

#ifdef _VERBOSE_DIREXT
    qDebug() << "QDirExt set its path to: " << root;
#endif

    // if a remote content directory, retrieve all content urls
    if (m_infoExt.isCopy() && m_infoExt.isDir()) {
#ifdef _VERBOSE_DIREXT
        qDebug() << "QDirExt is a directory, now populating its entries";
#endif
        // get the entries list
        QStringList entries = QDir::entryList();
        for (QList<QString>::Iterator i = entries.begin(); i != entries.end(); i++) {
            QString file = *i;
            QFileInfo entryInfo(file);
            QString type = entryInfo.suffix();
            if (type == "html" || type == "htm")
                addEntries(m_infoExt.getUrl(), root + QDir::separator() + file);
        }
    }
}

/**
  * Analyzes a root html content and lists its entries. The entries are
  * kept in the m_entries data member for later usage.
  */
void QDirExt::addEntries(QString root, const QString filename) {
#ifdef _VERBOSE_DIREXT
    qDebug() << "QDirExt(" << root << ") adding entries from file: " << filename;
#endif

    // open the html document and read all the content
    QFileExt fileExt(filename);
    fileExt.open(QFile::ReadOnly);
    QString content;
    do {
        char buff[1024];
        int len = fileExt.readLine(buff, sizeof(buff));
        if (len == -1)
            break;
        QByteArray line(buff, len);
        content += line;
    } while (true);
    fileExt.close();

    // look for href inside the content
    QRegExp hrefRegExp("href=[\\'\"]?([^\\'\" >]+)", Qt::CaseInsensitive);
    QStringList urls;
    int pos = 0;
    while ((pos = hrefRegExp.indexIn(content, pos)) != -1) {
        urls << hrefRegExp.cap(1);
        pos += hrefRegExp.matchedLength();
    }

    // keep the urls that are under the dir' path
    QUrl rootUrl(root);
    for (QList<QString>::iterator i = urls.begin(); i != urls.end(); i++) {
        QString path = *i;
        QUrl url(path);
        if (rootUrl.isParentOf(url) || url.scheme().isEmpty()) {
            path = url.path();
            if (path.startsWith("/"))
                path.remove(0, 1);
            if (!path.isEmpty() && !m_entries.contains(path)) {
#ifdef _VERBOSE_DIREXT
                qDebug() << "QDirExt(" << root << ") adding entry path: " << path;
#endif
                m_entries.append(path);
            }
        }
    }
}

/**
  * Returns a correct path separator for the given path, depending on its url scheme.
  */
QChar QDirExt::separator(QString path) {
    QUrl    url(path);
    QString scheme = url.scheme();
    if (scheme.isEmpty() || scheme == "file")
        return QDir::separator();

    return '/';
}

/**
  * Returns the entries list for this root local or remote directory.
  */
const QStringList QDirExt::entryList(Filters filters, SortFlags sort) {
    return m_infoExt.isCopy() ? m_entries : QDir::entryList(filters, sort);
}
