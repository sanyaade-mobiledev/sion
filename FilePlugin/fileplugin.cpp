/*
 * SION! Server basic file plugin.
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

#include <QFileInfo>
#include <QDebug>
#include <QDateTime>
#include <QtCore/qplugin.h>
#include <QFile>
#include <QRegExp>
#include <QDir>

#include "fileplugin.h"
#include "scriptrunner.h"

QMap<QString, AttributeCacheEntry *> FilePlugin::m_attributesCache;

PluginInterface *FilePlugin::newInstance(QString virtualDirectoryPath) {
    FilePlugin *newInstanceP = new FilePlugin();
    newInstanceP->initialize(virtualDirectoryPath);
    return newInstanceP;
}

void FilePlugin::initialize(QString virtualDirectoryPath) {
    m_virtualDirectoryPath = virtualDirectoryPath;
    m_scriptP = new Script("{\n\tplugin.setResult(true);\n}");
    m_result = false;

#ifdef _VERBOSE_FILE_PLUGIN
    qDebug() << "Base plugin initializes m_attributes";
#endif
    m_attributes.insert(PATH_ATTR, new Attribute(PATH_ATTR, tr("Fully qualified filename"), "String"));
    m_attributes.insert(NAME_ATTR, new Attribute(NAME_ATTR, tr("Name without path"), "String"));
    m_attributes.insert(TYPE_ATTR, new Attribute(TYPE_ATTR, tr("File extension (without the '.')"), "String"));
    m_attributes.insert(SIZE_ATTR, new Attribute(SIZE_ATTR, tr("Size of the file in bytes"), "Numeric"));
    m_attributes.insert(CREATED_ATTR, new Attribute(CREATED_ATTR, tr("Creation date"), "Date"));
    m_attributes.insert(MODIFIED_ATTR, new Attribute(MODIFIED_ATTR, tr("Last modification date"), "Date"));
    m_attributes.insert(READ_ATTR, new Attribute(READ_ATTR, tr("Last read date"), "Date"));
    m_attributes.insert(LINK_ATTR, new Attribute(LINK_ATTR, tr("Set if file is a symbolic link"), "Boolean"));
}

/**
  * Runs the current script.
  */
inline bool FilePlugin::runScript() {
    return m_scripter.run(m_scriptP, this);
}


/**
 * Runs the rules against the passed file.
 */
inline bool FilePlugin::checkFile(QString filepath) {
    loadAttributes(filepath);
    return runScript();
}

/**
 * attribute inspection (easier than reflection huh?)
 */
inline QVariant FilePlugin::getAttributeValue(QString attributeName) {
    QMap<QString, Attribute *>::const_iterator i = m_attributes.find(attributeName);
    if (i != m_attributes.end()) {
        Attribute *attrP = *i;
        return attrP->m_value;
    }

    return QVariant();
}

inline void FilePlugin::setAttributeValue(QString attributeName, QVariant value) {
    QMap<QString, Attribute *>::const_iterator i = m_attributes.find(attributeName);
    if (i != m_attributes.end()) {
        Attribute *attrP = *i;
        attrP->m_value = value;
    }
}

inline QString FilePlugin::getAttributeClassName(QString attributeName) {
    QMap<QString, Attribute *>::const_iterator i = m_attributes.find(attributeName);
    if (i != m_attributes.end()) {
        Attribute *attrP = *i;
        return attrP->m_className;
    }

    return QString();
}

inline QString FilePlugin::getAttributeTip(QString attributeName) {
    QMap<QString, Attribute *>::const_iterator i = m_attributes.find(attributeName);
    if (i != m_attributes.end()) {
        Attribute *attrP = *i;
        return attrP->m_tip;
    }

    return QString();
}

inline const QList<QString> FilePlugin::getAttributeNames() {
    return m_attributes.keys();
}

void FilePlugin::loadAttributes(QString filepath) {
#ifdef _VERBOSE_FILE_PLUGIN
    qDebug() << "loading attributes for file: " << filepath;
#endif

    // are the attributes in the cache?
    if (retrieveAttributesFromCache(filepath, FilePlugin::m_attributesCache))
        return;

    // get the file info.
    QFileInfo fileInfo(filepath);

    setAttributeValue(PATH_ATTR, fileInfo.absolutePath());
    setAttributeValue(TYPE_ATTR, fileInfo.suffix());
    setAttributeValue(NAME_ATTR, fileInfo.fileName());
    setAttributeValue(SIZE_ATTR, fileInfo.size());
    setAttributeValue(CREATED_ATTR, fileInfo.created());
    setAttributeValue(MODIFIED_ATTR, fileInfo.lastModified());
    setAttributeValue(READ_ATTR, fileInfo.lastRead());
    setAttributeValue(LINK_ATTR, fileInfo.isSymLink());

    // save attributes in the cache
    saveAttributesInCache(filepath, FilePlugin::m_attributesCache);
}

bool FilePlugin::contains(QString regExp) {
    bool result = false;

#ifdef _VERBOSE_FILE_PLUGIN
    qDebug() << "checking if the file contains the regexp: " << regExp;
#endif

    QRegExp exp(regExp);
    QString path = getAttributeValue(PATH_ATTR).toString();
    QString filename = getAttributeValue(NAME_ATTR).toString();
    QFile   file(path + QDir::separator() + filename);

    if(!file.open(QIODevice::ReadOnly))
        return false;

    // read the file content line by line and look for the regexp
    QTextStream in(&file);
    while(!result && !in.atEnd()) {
        QString line = in.readLine();
        if (exp.indexIn(line) >= 0)
            result = true;
    }

    file.close();

    return result;
}

/**
 * Saves the filepath file attributes stored in m_attributes in the attributesMapCache attributes
 * cache. If the file is in the cache and its last modification date hasn't changed, the cache entry
 * time is just updated. If the file is not in the cache or is in the cache but has been modified,
 * the entry is updated.
 * Before returning, the oldest cache entry is removed if the cache is full.
 */
void FilePlugin::saveAttributesInCache(QString filepath, QMap<QString, AttributeCacheEntry *> &attributesMapCache) {
    m_cacheSem.acquire();

    AttributeCacheEntry *entryP = NULL;

    QFileInfo info(filepath);
    QDateTime created = info.created();
    QDateTime modified = info.lastModified();
    QDateTime fileTime = created < modified ? modified : created; // doesn't seem to make much sense, but these values can be weird...

#ifdef _VERBOSE_FILE_PLUGIN
    qDebug() << "saving the file " << filepath << "'attributes in the cache";
#endif

    // is the file in the cache?
    QMap<QString, AttributeCacheEntry *>::Iterator i = attributesMapCache.find(filepath);
    if (i != attributesMapCache.end())  {
#ifdef _VERBOSE_FILE_PLUGIN
        qDebug() << "attributes are already in the cache";
#endif
        // update the entry
        entryP = *i;
        if (entryP->fileTime != fileTime)
            // the file was modified, update everything
            saveAttributes(entryP);
    } else {
#ifdef _VERBOSE_FILE_PLUGIN
        qDebug() << "attributes are now inserted in the cache";
#endif
        // add the file attributes into the cache
        entryP = new AttributeCacheEntry();
        saveAttributes(entryP);
        attributesMapCache.insert(filepath, entryP);
    }

    // update the cache entry and file times
    entryP->time = QDateTime::currentDateTime();
    entryP->fileTime = fileTime;

    // if the cache is full, remove the oldest value
    if (attributesMapCache.count() >= MAX_CACHED_ATTRIBUTES) {
        QDateTime oldestTime = QDateTime::currentDateTime();
        QString   oldestKey;

        QStringList keys = attributesMapCache.keys();
        for (QStringList::Iterator i = keys.begin(); i != keys.end(); i++) {
            QString key = *i;
            entryP = attributesMapCache.value(key);
            if (entryP && entryP->time < oldestTime) {
                oldestTime = entryP->time;
                oldestKey = key;
            }
        }

        if (!oldestKey.isEmpty()) {
#ifdef _VERBOSE_FILE_PLUGIN
            qDebug() << "removing the oldest file attributes from the cache: " << oldestKey;
#endif
            entryP = attributesMapCache.take(oldestKey);
            delete entryP;
        }
    }

    m_cacheSem.release();
}

/**
 * Retrieves in m_attributes the attributes for the filepath file from the attributesMapCache. Return true if
 * the attributes are present in the cache and up to date, else returns false. It true is returned, m_attributes
 * contains the file attributes.
 */
bool FilePlugin::retrieveAttributesFromCache(QString filepath, QMap<QString, AttributeCacheEntry *> &attributesMapCache) {
    m_cacheSem.acquire();

    bool                result = false;
    AttributeCacheEntry *entryP = NULL;

    QFileInfo info(filepath);
    QDateTime created = info.created();
    QDateTime modified = info.lastModified();
    QDateTime fileTime = created < modified ? modified : created; // doesn't seem to make much sense, but these values can be weird...

#ifdef _VERBOSE_FILE_PLUGIN
    qDebug() << "retrieving the file " << filepath << "'attributes from the cache";
#endif

    // is the file in the cache?
    QMap<QString, AttributeCacheEntry *>::Iterator i = attributesMapCache.find(filepath);
    if (i == attributesMapCache.end())
        goto endRetrieveAttributesFromCache;

    // is the cache up to date?
    entryP = *i;
    if (entryP->fileTime != fileTime)
        // the file was modified, update everything
        goto endRetrieveAttributesFromCache;

#ifdef _VERBOSE_FILE_PLUGIN
    qDebug() << "attributes are in the cache";
#endif

    // get the attributes
    loadAttributes(entryP);

    // update the cache entry time
    entryP->time = QDateTime::currentDateTime();

    result = true;

endRetrieveAttributesFromCache:
    m_cacheSem.release();

    return result;
}

void FilePlugin::saveAttributes(AttributeCacheEntry *entryP) {
    qDeleteAll(entryP->attributes);
    entryP->attributes.clear();
    QStringList attributes = m_attributes.keys();
    for (QStringList::Iterator i = attributes.begin(); i != attributes.end(); i++) {
        QString attrName = *i;
        Attribute *attrP = m_attributes.value(attrName);
        Attribute *saveAttrP = new Attribute(attrP->m_name); // we're caching only the name/value pairs
        saveAttrP->m_value = attrP->m_value;
        entryP->attributes.insert(attrName, saveAttrP);
    }
}

void FilePlugin::loadAttributes(AttributeCacheEntry *entryP) {
    QStringList attributes = entryP->attributes.keys();
    for (QStringList::Iterator i = attributes.begin(); i != attributes.end(); i++) {
        QString attrName = *i;
        Attribute *attrP = entryP->attributes.value(attrName);
        setAttributeValue(attrName, attrP->m_value);
    }
}


Q_EXPORT_PLUGIN2(FilePlugin, FilePlugin)

