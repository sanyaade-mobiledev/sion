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

#ifndef QFILEEXT_H
#define QFILEEXT_H

#include <QDir>
#include <QFile>
#include <QHttp>
#include <QUrl>
#include <QEventLoop>
#include <QDebug>
#include <QThread>

#include "QFileExtensions_global.h"


#define CACHE_DIRECTORY "RemoteContentCache"

//#define _VERBOSE_FILEEXT 1

/**
  * This class implement a remote http content synchronous downloader.
  * WARNING: contains an event loop that'll trigger slots in the instantiating/
  * calling thread.
  */
class SyncHttp : public QHttp {
    Q_OBJECT

public:
    explicit SyncHttp(QString host, QString page, QString filename) : QHttp(host) {
        setHost(host);
        m_page = page;
        m_filename = filename;
    }

    void downloadPage();

public slots:
    void done(bool error) {
        Q_UNUSED(error);

        if (error)
            qDebug() << "Synchronous HTTP get error: " << errorString();

        m_loop.exit();
    }

private:
    QString         m_page;
    QString         m_filename;
    QEventLoop      m_loop;

};

class SyncHttpThread : public QThread {
    Q_OBJECT

public:
    explicit SyncHttpThread(QString host, QString page, QString filename) {
        m_host = host;
        m_page = page;
        m_filename = filename;
    }

protected:
    void run() {
        // instantiate and execute the synchronous http getter
        // in the thread to avoid the event loop to trigger slots
        // in the parent process/thread.
        SyncHttp downloader(m_host, m_page, m_filename);
        downloader.downloadPage();
    }

private:
    QString m_host;
    QString m_page;
    QString m_filename;
};

/**
  * This class extends the QFile class to handle remote content. When
  * instanciated, the remote content is downloaded into the cache.
  */
class QFILEEXTENSIONSSHARED_EXPORT QFileExt : public QFile {
    Q_OBJECT

public:
    explicit QFileExt(const QString &url);

    bool isCopy() {
        return m_isCopy;
    }

    bool isDir() {
        return m_isDir;
    }

    QString getUrl() {
        return m_url;
    }

    static bool emptyAndRemove(QString path);

    static void downloadPage(QString host, QString page, QString filename);
    static bool compareFiles(QString filename1, QString filename2);

private:
    QString         m_url;          // original url
    bool            m_isCopy;       // set if this holds a copy of a remote content
    bool            m_isDir;        // true when referring to a remote content directory
};

#endif // QFILEEXT_H
