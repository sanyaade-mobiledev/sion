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

#ifndef SERVERDATABASE_H
#define SERVERDATABASE_H

#include <QSqlDatabase>
#include <QVector>
#include <QStringList>
#include <QSemaphore>

#include "ServerDatabase_global.h"

//#define _VERBOSE_DATABASE     1

#define _INSERT_UPDATE_ATTRIBUTE  1 // uses UPDATE clauses for tuple uniqueness, instead of DELETE + INSERT

#define _FILTER_INDEX               1
#define _FILE_INDEX                 1
#define _ATTRIBUTE_INDEX            1

#define DB_NAME "SIONDatabase"
#define DB_HOST "localhost"
#define DB_TYPE "QMYSQL"
#define DB_USR  "SION"
#define DB_PWD  "SION"

#define MAX_PATH_LEN            QString("512")
#define MAX_VIRTUAL_PATH_LEN    QString("256")
#define MAX_ATTR_NAME_LEN       QString("64")
#define MAX_ATTR_VALUE_LEN      QString("128")

/**
  * This class serves as an helper to access the MySQL SION! server database.
  */
class SERVERDATABASESHARED_EXPORT ServerDatabase {
public:
    ServerDatabase();
    virtual ~ServerDatabase();

    void                    cleanup(bool includingFilters = false);
    void                    addFilter(QString virtualDirectoryPath);
    QStringList             getFilters();
    void                    deleteFilter(QString filterId);
    QString                 getFilterId(QString virtualDirectoryPath);
    bool                    hasFile(QString filterId, QString filepath);
    QStringList             getFileAttributes(QString filterId, QString filepath);
    void                    addFileAttribute(QString fileId, QString attrName, QString attrValue);
    QString                 getFileAttribute(QString filterId, QString filepath, QString attrName);
    QString                 addFile(QString filterId, QString filepath);
    QStringList             getFiles(QString filterId);
    void                    removeFile(QString filterId, QString filepath);
    void                    removeFiles(QString filterId);

private:
    QSqlDatabase        m_db;
    static int          m_dbref;
    static QSemaphore   m_dbSem;

    void sanitizeString(QString &string);
    void unsanitizeString(QString &string);

    void createTables();
};

#endif // SERVERDATABASE_H
