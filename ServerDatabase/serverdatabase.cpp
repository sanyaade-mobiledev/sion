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

#include <QDebug>
#include <QSqlQuery>
#include <QStringList>
#include <QSqlError>
#include <QVector>
#include <QVariant>
#include <QLibrary>
#include <QPluginLoader>
#include <QSqlDriverPlugin>
#include <QSqlDriver>

#include "serverdatabase.h"

int         ServerDatabase::m_dbref = 0;
QSemaphore  ServerDatabase::m_dbSem(1);

/**
  * The constructor creates the db and the tables if required. There's a single instance of db for all
  * the ServerDatabase instances.
  */
ServerDatabase::ServerDatabase() {
#ifdef _VERBOSE_DATABASE
    qDebug() << "Creating a new instance of the db connector: " << m_dbref;
#endif
    if (!m_dbref++) {
        // manually load the driver (for some reason I couldn't figure out how to have it loaded automagically)
        QPluginLoader loader("libqsqlmysql.so");
        if (loader.load()) {
#ifdef _VERBOSE_DATABASE
            qDebug() << "Loaded mysql drivers plugin";
#endif
        } else {
            qDebug() << QObject::tr("Failed to load mysql drivers plugin: ") + loader.errorString();
            return;
        }

        QSqlDriverPlugin *sqlPlugin  = qobject_cast<QSqlDriverPlugin *>(loader.instance());
#ifdef _VERBOSE_DATABASE
        qDebug() << "Available sql drivers: " << sqlPlugin->keys();
#endif
        QSqlDriver *sqlDriver = sqlPlugin->create(DB_TYPE);
        if (!sqlDriver) {
            qDebug() << QObject::tr("Failed to instantiate mysql driver");
            return;
        }

        sqlDriver->open(DB_NAME, DB_USR, DB_PWD, DB_HOST);
        if (sqlDriver->isOpenError()) {
            qDebug() << QObject::tr("Failed to connect to (") + DB_TYPE + QObject::tr(") DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd '") + DB_USR + "/" + DB_PWD + "'";
            qDebug() << QObject::tr("ERROR: ") + sqlDriver->lastError().text();
            return;
        }
        m_db = QSqlDatabase::addDatabase(sqlDriver);

        // don't disconnect even if there's no interaction with the db server
        m_db.setConnectOptions("MYSQL_OPT_CONNECT_TIMEOUT=0");

        // create tables if non existing
        createTables();
    }
}

ServerDatabase::~ServerDatabase() {
#ifdef _VERBOSE_DATABASE
    qDebug() << "deleting an instance of the db connector (" << m_dbref << ")";
#endif
    if (--m_dbref > 0)
        return;

    // commit and close the db if opened
    if (m_db.isOpen()) {
        m_db.commit();
        m_db.close();
    }
}

/**
 * Create the "files" and "attributes" tables if they don't exist yet
 *
 */
void ServerDatabase::createTables() {
    if (!m_db.tables().isEmpty())
        return;

    QSqlQuery query(m_db);

#ifdef _VERBOSE_DATABASE
    qDebug() << "Creating table filters" << m_dbref;
#endif

    if (!query.exec("CREATE TABLE IF NOT EXISTS filters(filter_id BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY, virtual_directory VARCHAR(" + MAX_VIRTUAL_PATH_LEN + ")) ENGINE=MyISAM DEFAULT CHARSET=latin1")) {
        qDebug() << QObject::tr("Failed to create 'filters' table in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
        qDebug() << QObject::tr("ERROR: ") + query.lastError().text();
        return;
    }

#ifdef _FILTER_INDEX
    if (!query.exec("CREATE INDEX virtual_directory ON filters(virtual_directory)")) {
        qDebug() << QObject::tr("Failed to create index in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
        qDebug() << QObject::tr("ERROR: ") + query.lastError().text();
        return;
    }
#endif

#ifdef _VERBOSE_DATABASE
    qDebug() << "Creating table files" << m_dbref;
#endif

    if (!query.exec("CREATE TABLE IF NOT EXISTS files(file_id BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY, filter_id BIGINT NOT NULL, path VARCHAR(" + MAX_PATH_LEN + ")) ENGINE=MyISAM DEFAULT CHARSET=latin1")) {
        qDebug() << QObject::tr("Failed to create 'files' table in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
        qDebug() << QObject::tr("ERROR: ") + query.lastError().text();
        return;
    }

#ifdef _FILE_INDEX
    if (!query.exec("CREATE INDEX filepath_filterid_pair ON files(path, filter_id)")) {
        qDebug() << QObject::tr("Failed to create index in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
        qDebug() << QObject::tr("ERROR: ") + query.lastError().text();
        return;
    }
#endif

#ifdef _VERBOSE_DATABASE
    qDebug() << "Creating table attributes" << m_dbref;
#endif

    // retained files' attributes
    if (!query.exec("CREATE TABLE IF NOT EXISTS attributes(file_id BIGINT NOT NULL, attribute_name VARCHAR(" + MAX_ATTR_NAME_LEN + "), attribute_value VARCHAR(" + MAX_ATTR_VALUE_LEN + ")) ENGINE=MyISAM DEFAULT CHARSET=latin1")) {
        qDebug() << QObject::tr("Failed to create 'attributes' table in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
        qDebug() << QObject::tr("ERROR: ") + query.lastError().text();
        return;
    }
#ifdef _ATTRIBUTE_INDEX
#ifdef _INSERT_UPDATE_ATTRIBUTE
    if (!query.exec("CREATE UNIQUE INDEX fileid_attribute_pair ON attributes(file_id, attribute_name)")) {
#else
    if (!query.exec("CREATE INDEX fileid_attribute_pair ON attributes(file_id, attribute_name)")) {
#endif
        qDebug() << QObject::tr("Failed to create index in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
        qDebug() << QObject::tr("ERROR: ") + query.lastError().text();
        return;
    }
#endif

#ifdef _VERBOSE_DATABASE
    qDebug() << "Creating indexes" << m_dbref;
#endif
}

/**
 * Performs a full db cleanup (of the files related tables, not the filters table
 * which is completely managed by the filters).
 */
void ServerDatabase::cleanup(bool includingFilters) {
    m_dbSem.acquire();

    QSqlQuery query(m_db);

#ifdef _VERBOSE_DATABASE
    qDebug() << "Cleaning up tables" << m_dbref;
#endif

    // delete files and tables tuples, don't care about the result since there isn't much we can
    // do if this fails. The auto-increment ID will restart from 0.
    query.exec("DELETE FROM files");
    query.exec("ALTER TABLE files AUTO_INCREMENT=0");
    query.exec("DELETE FROM attributes");

    if (includingFilters) {
        query.exec("DELETE FROM filters");
        query.exec("ALTER TABLE filters AUTO_INCREMENT=0");
    }

    m_dbSem.release();
}

/**
 * Search for a file reference in the db
 *
 * @param filterId is the filter id
 * @param filepath is the full pathname of the new file
 * @return file reference was found
 */
bool ServerDatabase::hasFile(QString filterId, QString filepath) {
    bool result = FALSE;

    m_dbSem.acquire();

    QSqlQuery query(m_db);

    sanitizeString(filepath);

#ifdef _VERBOSE_DATABASE
    qDebug() << "Checking if " << filterId << "/" << filepath << " have file(s) in db";
#endif

    // check if file exists
    if (!query.exec("SELECT * FROM files WHERE path='" + filepath + "' AND filter_id=" + filterId))
        goto hasFileEnd;

    // if the result set is not empty, we can move to the next selected tuple...
    if (query.next())
            result = TRUE;

hasFileEnd:
    m_dbSem.release();

#ifdef _VERBOSE_DATABASE
    qDebug() << "Checking if " << filterId << "/" << filepath << " have file(s) in db reports: " << result;
#endif

    return result;
}

/**
 * Gets the file attribute names/values. Returns a QStringList of attribute names.
 *
 * @param filterId is the filter id
 * @param filepath is the full pathname of the file
 */
QStringList ServerDatabase::getFileAttributes(QString filterId, QString filepath) {
    m_dbSem.acquire();

    QStringList result;

    QSqlQuery query(m_db);

    sanitizeString(filepath);

#ifdef _VERBOSE_DATABASE
    qDebug() << "getting file attributes for " << filterId << "/" << filepath;
#endif

    // get all tuples vdir/File
    query.exec("SELECT attributes.attribute_name FROM files, attributes WHERE files.path='" + filepath + "' AND files.file_id=attributes.file_id AND files.filter_id=" + filterId);
    while (query.next()) {
        QString name = query.value(0).toString();
        unsanitizeString(name);

        result += name;  // "attribute_name" column value

#ifdef _VERBOSE_DATABASE
        qDebug() << "retrieved file attribute name for " << filterId << "/" << filepath << ": " << name;
#endif
    }

    m_dbSem.release();

    return result;
}

/**
 * Gets a file attribute value from the db
 *
 * @param filterId is the filter id
 * @param filepath is the full pathname of the new file
 * @param attrName is the name of the file attribute
 */
QString ServerDatabase::getFileAttribute(QString filterId, QString filepath, QString attrName) {
    QString result;

    m_dbSem.acquire();

    QSqlQuery query(m_db);

    sanitizeString(filepath);
    sanitizeString(attrName);

#ifdef _VERBOSE_DATABASE
        qDebug() << "retrieving file attribute " << filterId << "/" << filepath << "/" << attrName;
#endif

    // select tuple fiterId/filepath/attrName if existing
    if (!query.exec("SELECT attributes.attribute_value FROM files, attributes WHERE files.path='" + filepath + "' AND files.filter_id=" + filterId + " AND files.file_id=attributes.file_id AND attribute_name='" + attrName + "'")) {
        qDebug() << QObject::tr("Failed to select from attributes table in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
        qDebug() << QObject::tr("ERROR: ") + query.lastError().text();
    } else if (query.next()) {
        result = query.value(0).toString();
        unsanitizeString(result);

#ifdef _VERBOSE_DATABASE
        qDebug() << "retrieved file attribute " << filterId << "/" << filepath << "/" << attrName << ": " << result;
#endif
    }

    m_dbSem.release();

    return result;
}

/**
 * Adds a new (unique) file attribute pair (name/value) to the db
 *
 * @param fileId is the file id
 * @param attrName is the name of the file attribute
 * @param attrValue is the value of the file attribute
 */
void ServerDatabase::addFileAttribute(QString fileId, QString attrName, QString attrValue) {
    m_dbSem.acquire();

    QSqlQuery query(m_db);

    sanitizeString(attrName);
    sanitizeString(attrValue);

#ifdef _VERBOSE_DATABASE
        qDebug() << "adding file attribute " << fileId << "/" << attrName;
#endif

    // insert tuple filepath/attrName
#ifndef _INSERT_UPDATE_ATTRIBUTE
    query.exec("DELETE FROM attributes WHERE file_id=" + fileId + " AND attribute_name='" + attrName + "'");
    if (!query.exec("INSERT INTO attributes(file_id, attribute_name, attribute_value) VALUES(" + fileId + ", '" + attrName + "', '" + attrValue + "')")) {
#else
    if (!query.exec("INSERT INTO attributes(file_id, attribute_name, attribute_value) VALUES(" + fileId + ", '" + attrName + "', '" + attrValue + "') ON DUPLICATE KEY UPDATE attribute_value='" + attrValue + "'")) {
#endif
        qDebug() << QObject::tr("Failed to insert into attributes table in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
        qDebug() << QObject::tr("ERROR: ") + query.lastError().text();
    }

    m_dbSem.release();
}

/**
 * Gets all file references from the db for a given filter.
 *
 * @param filterId is the filter id
 */
QStringList ServerDatabase::getFiles(QString filterId) {
    m_dbSem.acquire();

    QStringList result;

    QSqlQuery query(m_db);

#ifdef _VERBOSE_DATABASE
        qDebug() << "retrieving files for " << filterId;
#endif

    // get files
    if (!query.exec("SELECT path FROM files WHERE filter_id=" + filterId)) {
        qDebug() << QObject::tr("Failed to query from files table in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
        qDebug() << QObject::tr("ERROR: ") + query.lastError().text();
    } else {
        while (query.next()) {
            QString filepath = query.value(0).toString();
            unsanitizeString(filepath);

#ifdef _VERBOSE_DATABASE
            qDebug() << "retrieved file for " << filterId << ": " << filepath;
#endif
            result += filepath;
        }
    }

    m_dbSem.release();

    return result;
}


/**
 * Adds a new (unique) file reference to the db
 *
 * @param filterId is the filter id
 * @param filepath is the full pathname of the new file
 * @return the file id of the new file
 */
QString ServerDatabase::addFile(QString filterId, QString filepath) {
    m_dbSem.acquire();

    QString fileId;
    QSqlQuery query(m_db);

    sanitizeString(filepath);

#ifdef _VERBOSE_DATABASE
    qDebug() << "adding file " << filterId << "/" << filepath;
#endif

    // check if file exists
    query.exec("SELECT file_id FROM files WHERE path='" + filepath + "' AND filter_id=" + filterId);
    if (query.next())
        fileId = query.value(0).toString();
    else {
        if (!query.exec("INSERT INTO files(path, filter_id) VALUES('" + filepath + "', " + filterId + ")")) {
            qDebug() << QObject::tr("Failed to insert (") << filterId << ", " << filepath << QObject::tr(") into the files table.");
            qDebug() << QObject::tr("ERROR: ") << query.lastError().text();
        } else {
            query.exec("SELECT file_id FROM files WHERE path='" + filepath + "' AND filter_id=" + filterId);
            if (query.next())
                fileId = query.value(0).toString();
        }
    }

    m_dbSem.release();

    return fileId;
}

/**
 * Removes a file reference from the db
 *
 * @param filterId is the filter id
 * @param filepath is the full pathname of the new file
 */
void ServerDatabase::removeFile(QString filterId, QString filepath) {
    m_dbSem.acquire();

    QSqlQuery query(m_db);

    sanitizeString(filepath);

#ifdef _VERBOSE_DATABASE
    qDebug() << "removing file " << filterId << "/" << filepath;
#endif

    // retrieve file_id for the given file, filterId
    if (!query.exec("SELECT file_id FROM files WHERE path='" + filepath + "' AND filter_id=" + filterId)) {
        qDebug() << QObject::tr("Failed to delete from attributes table in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
        qDebug() << QObject::tr("ERROR: ") + query.lastError().text();
    } else if (query.next()) {
        QString fileId = query.value(0).toString();

        if (!query.exec("DELETE FROM files WHERE file_id=" + fileId)) {
            qDebug() << QObject::tr("Failed to delete from files table in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
            qDebug() << QObject::tr("ERROR: ") + query.lastError().text();
        }

        if (!query.exec("DELETE FROM attributes WHERE file_id=" + fileId)) {
            qDebug() << QObject::tr("Failed to delete from attributes table in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
            qDebug() << QObject::tr("ERROR: ") + query.lastError().text();
        }
    }

    m_dbSem.release();
}

/**
 * Removes all file references from the db
 *
 * @param filterId is the filter id
 */
void ServerDatabase::removeFiles(QString filterId) {
    m_dbSem.acquire();

    QSqlQuery query(m_db);
    QSqlQuery delQuery(m_db);

#ifdef _VERBOSE_DATABASE
    qDebug() << "removing files for " << filterId;
#endif

    // retrieve file_ids for the given filterId
    if (!query.exec("SELECT file_id FROM files WHERE filter_id=" + filterId)) {
        qDebug() << QObject::tr("Failed to delete from files and attributes table in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
        qDebug() << QObject::tr("ERROR: ") + query.lastError().text();
    } else {
        while (query.next()) {
            QString fileId = query.value(0).toString();

#ifdef _VERBOSE_DATABASE
            qDebug() << "removing file " << filterId << "/" << fileId;
#endif

            if (!delQuery.exec("DELETE FROM files WHERE file_id=" + fileId)) {
                qDebug() << QObject::tr("Failed to delete from files and attributes table in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
                qDebug() << QObject::tr("ERROR: ") + delQuery.lastError().text();
            }

            if (!delQuery.exec("DELETE FROM attributes WHERE file_id=" + fileId)) {
                qDebug() << QObject::tr("Failed to delete from files and attributes table in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
                qDebug() << QObject::tr("ERROR: ") + delQuery.lastError().text();
            }
        }
    }

    m_dbSem.release();
}

/**
  * Sanitize: replace "'" substrings in filepaths by "%Q%"
  * Unsanitize: replace "%Q%" substrings in filepaths by "'".
  *
  * @param string is the string to be sanitized/unsanitized
  */
void ServerDatabase::sanitizeString(QString &string) {
    string.replace(QString("'"), QString("%27"));
    string.replace(QString("`"), QString("%2C"));
}

void ServerDatabase::unsanitizeString(QString &string) {
    string.replace(QString("%27"), QString("'"));
    string.replace(QString("%2C"), QString("`"));
}

/**
  * Adds a filter virtual directory path to the filters database
  * if not already there.
  *
  * @param virtualDirectoryPath is the filter path
  */
void ServerDatabase::addFilter(QString virtualDirectoryPath) {
    m_dbSem.acquire();

    QSqlQuery query(m_db);

    sanitizeString(virtualDirectoryPath);

#ifdef _VERBOSE_DATABASE
    qDebug() << "adding filter " << virtualDirectoryPath;
#endif

    // check if filter exists
    query.exec("SELECT * FROM filters WHERE virtual_directory='" + virtualDirectoryPath + "'");
    if (!query.next()) {
        if (!query.exec("INSERT INTO filters(virtual_directory) VALUES('" + virtualDirectoryPath + "')")) {
            qDebug() << QObject::tr("Failed to insert (") << virtualDirectoryPath << QObject::tr(") into the filters table.");
            qDebug() << QObject::tr("ERROR: ") << query.lastError().text();
        }
    }

    m_dbSem.release();
}

/**
 * Gets all filters from the db.
 */
QStringList ServerDatabase::getFilters() {
    m_dbSem.acquire();

    QStringList result;

    QSqlQuery query(m_db);

#ifdef _VERBOSE_DATABASE
        qDebug() << "retrieving filters";
#endif

    // get filters
    if (!query.exec("SELECT virtual_directory FROM filters")) {
        qDebug() << QObject::tr("Failed to query from filters table in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
        qDebug() << QObject::tr("ERROR: ") + query.lastError().text();
    } else {
        while (query.next()) {
            QString filter = query.value(0).toString();
            unsanitizeString(filter);

#ifdef _VERBOSE_DATABASE
            qDebug() << "retrieved filter " << filter;
#endif
            result += filter;
        }
    }

    m_dbSem.release();

    return result;
}

/**
  * Deletes a filter virtual directory path from the filters database.
  *
  * @param filterId is the filter id
  */
void ServerDatabase::deleteFilter(QString filterId) {
    m_dbSem.acquire();

    QSqlQuery query(m_db);

#ifdef _VERBOSE_DATABASE
    qDebug() << "removing filter " << filterId;
#endif

    if (!query.exec("DELETE FROM filters WHERE filter_id=" + filterId)) {
        qDebug() << QObject::tr("Failed to delete from filters table in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
        qDebug() << QObject::tr("ERROR: ") + query.lastError().text();
    }

    m_dbSem.release();
}

/**
  * Retrieves a filter id from its virtual directory path from the filters database.
  *
  * @param virtualDirectoryPath is the filter virtual path
  * @return the associated filter id or an empty string if not found
  */
QString ServerDatabase::getFilterId(QString virtualDirectoryPath) {
    QString filterId;

    m_dbSem.acquire();

    QSqlQuery query(m_db);

    sanitizeString(virtualDirectoryPath);

#ifdef _VERBOSE_DATABASE
    qDebug() << "retrieving filter id for " << virtualDirectoryPath;
#endif

    // retrieve filter_id for the given virtualDirectoryPath
    if (!query.exec("SELECT filter_id FROM filters WHERE virtual_directory='" + virtualDirectoryPath + "'")) {
        qDebug() << QObject::tr("Failed to select from filters table in DB ") + DB_NAME + QObject::tr(" on host ") + DB_HOST + QObject::tr(" with usr/pwd ") + DB_USR + "/" + DB_PWD;
        qDebug() << QObject::tr("ERROR: ") + query.lastError().text();
    } else {
        query.next();
        filterId = query.value(0).toString();

#ifdef _VERBOSE_DATABASE
        qDebug() << "retrieved filter id for " << virtualDirectoryPath << ": " << filterId;
#endif
    }

    m_dbSem.release();

    return filterId;
}
