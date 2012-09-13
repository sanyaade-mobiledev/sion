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

PluginInterface *FilePlugin::newInstance(QString virtualDirectoryPath) {
    FilePlugin *newInstanceP = new FilePlugin();
    newInstanceP->initialize(virtualDirectoryPath);
    return newInstanceP;
}

void FilePlugin::initialize(QString virtualDirectoryPath) {
    m_virtualDirectoryPath = virtualDirectoryPath;
    m_scriptP = new Script("{\n\tplugin.setResult(true);\n}");
    m_result = false;

#ifdef _VERBOSE_PLUGIN
    qDebug() << "Base plugin initializes m_attributes";
#endif
    m_attributes.insert(PATH_ATTR, new Attribute(PATH_ATTR, tr("Fully qualified filename"), "String"));
    m_attributes.insert(NAME_ATTR, new Attribute(NAME_ATTR, tr("Name without path"), "String"));
    m_attributes.insert(TYPE_ATTR, new Attribute(TYPE_ATTR, tr("File extension (without the '.')"), "String"));
    m_attributes.insert(SIZE_ATTR, new Attribute(SIZE_ATTR, tr("size of the file in bytes"), "Numeric"));
    m_attributes.insert(CREATED_ATTR, new Attribute(CREATED_ATTR, tr("Creation date"), "Date"));
    m_attributes.insert(MODIFIED_ATTR, new Attribute(MODIFIED_ATTR, tr("Last modification date"), "Date"));
    m_attributes.insert(READ_ATTR, new Attribute(READ_ATTR, tr("Last read date"), "Date"));
    m_attributes.insert(LINK_ATTR, new Attribute(LINK_ATTR, tr("Set if file is a symbolic link"), "Boolean"));
}

/**
  * Runs the current script.
  */
bool FilePlugin::runScript() {
    return m_scripter.run(m_scriptP, this);
}


/**
 * Runs the rules against the passed file.
 */
bool FilePlugin::checkFile(QString filepath) {
    loadAttributes(filepath);
    return runScript();
}

/**
 * attribute inspection (easier than reflection huh?)
 */
QVariant FilePlugin::getAttributeValue(QString attributeName) {
    QMap<QString, Attribute *>::const_iterator i = m_attributes.find(attributeName);
    if (i != m_attributes.end()) {
        Attribute *attrP = *i;
        return attrP->m_value;
    }

    return QVariant();
}

void FilePlugin::setAttributeValue(QString attributeName, QVariant value) {
    QMap<QString, Attribute *>::const_iterator i = m_attributes.find(attributeName);
    if (i != m_attributes.end()) {
        Attribute *attrP = *i;
        attrP->m_value = value;
    }
}

QString FilePlugin::getAttributeClassName(QString attributeName) {
    QMap<QString, Attribute *>::const_iterator i = m_attributes.find(attributeName);
    if (i != m_attributes.end()) {
        Attribute *attrP = *i;
        return attrP->m_className;
    }

    return QString();
}

QString FilePlugin::getAttributeTip(QString attributeName) {
    QMap<QString, Attribute *>::const_iterator i = m_attributes.find(attributeName);
    if (i != m_attributes.end()) {
        Attribute *attrP = *i;
        return attrP->m_tip;
    }

    return QString();
}

const QList<QString> FilePlugin::getAttributeNames() {
    return m_attributes.keys();
}

void FilePlugin::forceLoadAttributes(QString filepath) {
#ifdef _VERBOSE_PLUGIN
    qDebug() << "force loading attributes for file: " << filepath;
#endif

    m_lastLoadedFile = "";

    loadAttributes(filepath);
}

void FilePlugin::loadAttributes(QString filepath) {
#ifdef _VERBOSE_PLUGIN
    qDebug() << "loading attributes for file: " << filepath;
#endif

    // already loaded?
    if (m_lastLoadedFile == filepath) {
#ifdef _VERBOSE_PLUGIN
        qDebug() << "already loaded attributes for file: " << filepath;
#endif
        return;
    }

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

    m_lastLoadedFile = filepath;
}

bool FilePlugin::contains(QString regExp) {
    bool result = false;

#ifdef _VERBOSE_PLUGIN
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

Q_EXPORT_PLUGIN2(FilePlugin, FilePlugin)

