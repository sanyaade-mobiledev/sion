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
#include <QHttp>
#include <QCoreApplication>
#include <QDir>

#include "qfileext.h"

/**
  * Empty the given directory (rm -r <path>).
  */
bool QFileExt::emptyAndRemove(QString path) {
    bool result = false;

    QDir dir(path);

    if (dir.exists(path)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir())
                result = emptyAndRemove(info.absoluteFilePath());
            else
                result = QFile::remove(info.absoluteFilePath());

            if (!result)
                return result;
        }

        result = dir.rmdir(path);
    }

    return result;
}

/**
  * Constructs a QFileExt based on the given URL. If remote, the
  * corresponding content is downloaded into the cache. So far, only remote
  * http scheme and html content is supported.
  */
QFileExt::QFileExt(const QString &url) : QFile() {
#ifdef _VERBOSE_FILEEXT
    qDebug() << "instanciating QFileExt(" << url << ")";
#endif

    m_isCopy = false;
    m_isDir = false;
    m_url = url;

    // is the file local?
    QUrl    fullUrl(url);
    QString page = fullUrl.path();
    QString scheme = fullUrl.scheme();

    if (scheme.isEmpty() || scheme == "file") {
#ifdef _VERBOSE_FILEEXT
        qDebug() << "QFileExt is local";
#endif
        // just set the filename
        setFileName(page);
    } else if (scheme == "http") {
#ifdef _VERBOSE_FILEEXT
        qDebug() << "QFileExt is a remote content copy";
#endif
        // http url
        m_isCopy = true;

        // create a local copy of the remote content
        QString host = fullUrl.host();

        // get the name of the directory that'll contain the file
        QString dirPath = CACHE_DIRECTORY;
        dirPath += QDir::separator() + host;
        dirPath.replace("/", QDir::separator());

        // create the directory if it doesn't exist
        QDir dir;
        dir.mkpath(dirPath);

        // if the page name has a path, all the directories down to the
        // page must be created, else we'll get the urls, and we'll download
        // the pages... to invalid filenames. fixme!!!
        // e.g: //europe/crise/finance/bla bla bla.html
        if (page.contains("/")) {
            QString pathSegment = dirPath;
            QStringList pagePath = page.split("/");
            for (int i = 0; i < pagePath.count() - 1; i++) {
                if (!pagePath.at(i).isEmpty()) {
                    pathSegment += QDir::separator();
                    pathSegment += pagePath.at(i);
                    dir.mkpath(pathSegment);
                }
            }
        }

#ifdef _VERBOSE_FILEEXT
        qDebug() << "QFileExt created directory: " << dirPath;
#endif
        // build the target web page copy filename
        m_isDir = page.length() <= 1; // no page specified, the file is a directory
        QString filename = dirPath;
        filename += QDir::separator();
        if (m_isDir)
            filename += "index.html";
        else
            filename += page.right(page.length() - 1);

#ifdef _VERBOSE_FILEEXT
        qDebug() << "QFileExt will load: " << filename;
#endif
        // loads the remote content into the file or a new copy if it already exists
        QFileInfo fileInfo(filename);
        if (fileInfo.exists()) {
            // the copy already exists, check modifications
            downloadPage(host, page, filename + ".new");

            // if the page wasn't modified, keep the old one
            // else, remove the old copy and replace it by the new one
            if (compareFiles(filename, filename + ".new")) {
                QFile(filename + ".new").remove();

#ifdef _VERBOSE_FILEEXT
                qDebug() << "QFileExt kept an unchanged page copy";
#endif

            } else {
                QFile(filename).remove();
                QFile(filename + ".new").rename(filename);

#ifdef _VERBOSE_FILEEXT
                qDebug() << "QFileExt replaced a changed page copy";
#endif
            }

        } else {
            // the copy doesnt exist, get it
            downloadPage(host, page, filename);
        }

        // keep the file or directory reference
        setFileName(m_isDir ? dirPath : filename);
    }
}

/**
  * Download the given remote content into the local filename file.
  */
void QFileExt::downloadPage(QString host, QString page, QString filename) {
#ifdef _VERBOSE_FILEEXT
    qDebug() << "QFileExt's downloader thread will start";
#endif

    SyncHttpThread thread(host, page, filename);
    thread.start();
    thread.wait();

#ifdef _VERBOSE_FILEEXT
    qDebug() << "QFileExt's downloader thread terminated";
#endif
}

/**
  * Synchronous HTTP download.
  */
void SyncHttp::downloadPage() {
#ifdef _VERBOSE_FILEEXT
    qDebug() << "QFileExt's downloader will downloaded page " << m_page << " into: " << m_filename;
#endif

    // connect to the http end request signal and
    connect(this, SIGNAL(done(bool)), SLOT(done(bool)));

    QFile file(m_filename);
    get(m_page.isEmpty() ? "/" : m_page, &file);

    // wait for the content to be read completely
    // note: this is absolutely ugly but I didn't find a way to do
    // it better :/
    m_loop.exec();

#ifdef _VERBOSE_FILEEXT
    qDebug() << "QFileExt's downloader just downloaded page " << m_page << " into: " << m_filename;
#endif
}

/**
  * Returns (filename1 file content == filename2 file content).
  */
bool QFileExt::compareFiles(QString filename1, QString filename2) {
    bool result = true;

    QFile file1(filename1);
    QFile file2(filename2);

    file1.open(QFile::ReadOnly);
    file2.open(QFile::ReadOnly);

    int i1, i2;
    do {
        char c1;
        char c2;
        i1 = file1.read(&c1, 1);
        i2 = file2.read(&c2, 1);
        result &= ((i1 == i2) &&
                   (c1 == c2));
    } while (result && i1 && i2);

    file1.close();
    file2.close();

    return result;
}
