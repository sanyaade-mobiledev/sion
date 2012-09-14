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

#include <QFile>
#include <QFileInfo>

#include "classifier.h"
#include "serverdatabase.h"

Classifier::Classifier(QObject *parentP) : QObject(parentP) {
}


/**
  * Reads a utf8 string from the given data stream. This is done
  * by reading the string size (including the trailing 0) then
  * the string.
  */
QString Classifier::readUtf8String(QDataStream &in) {
    uint       length;
    char       *dataP;

    in >> length;
    in.readBytes(dataP, length);

    QString string = QString::fromUtf8((const char *)dataP);
    delete dataP;

    return string;
}

/**
  * Writes a utf8 string to the given data stream. This is done
  * by writing the string size (including the trailing 0) then
  * the string.
  */
void Classifier::writeUtf8String(QDataStream &out, QString string) {
    QByteArray data(string.toUtf8());

    uint length = data.length();
    out << length;
    out.writeBytes(data.constData(), length);
}

/**
  * Adds a filter to the set of current filters handled by this classifier. Returns the
  * newly created filter.
  */
Filter *Classifier::addFilter(Filter *parentP, QString virtualDirectoryPath, QString dir, bool recursive, QStringList plugins, bool dontStart) {
    Filter *filterP = new Filter(virtualDirectoryPath, dir, recursive, plugins, parentP);
    if (parentP)
        parentP->addChild(filterP);

    m_filters.append(filterP);

    if (!dontStart)
        filterP->start();

    // signals
    newFilter(virtualDirectoryPath);

    return filterP;
}

/**
 * Removes a filter from the currently installed filters.
 */
void Classifier::removeFilter(Filter *filterP) {
    if (!filterP || !m_filters.contains(filterP))
        return;

    // signals
    delFilter(filterP->getVirtualDirectoryPath());

    // remove filter

    // stop it and its children
    filterP->stop();

    // cleanup all retained files for this filter and its children
    filterP->cleanup();

    // remove all filter's children
    filterP->deleteChildren(&m_filters);

    // remove this filter from the filters list
    int fIndex = m_filters.indexOf(filterP);
    if (fIndex != -1)
        m_filters.remove(fIndex);

    // remove this filter from its parent..
    Filter *parentP = filterP->getParent();
    if (parentP)
        parentP->removeChild(filterP);

    delete filterP;
}

/**
  * Returns the filter with the given virtual directory path or NULL if not found.
  */
Filter *Classifier::findFilter(QString virtualDirectoryPath) {
    for (int i = 0; i < m_filters.count(); i++)
        if (m_filters[i]->getVirtualDirectoryPath() == virtualDirectoryPath)
            return m_filters[i];

    return NULL;
}

/**
  * Starts/Stops the root filters.
  */
void Classifier::start() {
#ifdef _VERBOSE_CLASSIFIER
    qDebug() << "Classifier will start";
#endif

    // iterate through all root filters and start
    for (int i = 0; i < m_filters.count(); i++)
        if (m_filters[i]->isRoot())
            m_filters[i]->start();
}

void Classifier::stop() {
#ifdef _VERBOSE_CLASSIFIER
    qDebug() << "Classifier will stop";
#endif

    // iterate through all root filters and stop
    for (int i = 0; i < m_filters.count(); i++)
        if (m_filters[i]->isRoot())
            m_filters[i]->stop();
}

/**
  * Rescan (db cleanup + scan) all the root filters.
  */
void Classifier::rescan() {
    // iterate through all root filters and rescan
    for (int i = 0; i < m_filters.count(); i++)
        if (m_filters[i]->isRoot())
            m_filters[i]->rescanDirectory();
}

/**
  * Scan all the root filters.
  */
void Classifier::scan() {
    // iterate through all root filters and scan
    for (int i = 0; i < m_filters.count(); i++)
        if (m_filters[i]->isRoot())
            m_filters[i]->scanDirectory();
}

/**
  * Save all the filters in the given file.
  */
void Classifier::saveFilters(QString filename) {
    QFile file(filename);

#ifdef _VERBOSE_CLASSIFIER
    qDebug() << "Classifier will save filterset " << filename;
#endif

    // open the file and set up the data stream
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate))
        return;

    QDataStream out(&file);

    displayActivity(tr("Saving filters"));

    int     numFilters = m_filters.count();
    QString numFiltersStr;
    numFiltersStr.sprintf("%d", numFilters);
    writeUtf8String(out, numFiltersStr);

    // save filters
    for (int i = 0; i < numFilters; i++) {
        Filter *fP = m_filters[i];
        QString virDirPath = fP->getVirtualDirectoryPath();
        writeUtf8String(out, fP->getUrl());
        writeUtf8String(out, (QString)(fP->getParent() ? fP->getParent()->getVirtualDirectoryPath() : ""));
        writeUtf8String(out, virDirPath);
        writeUtf8String(out, (QString)(fP->isRecursive() ? "TRUE" : "FALSE"));

        displayActivity(tr("Saving %1").arg(virDirPath));
        displayProgress(0, numFilters - 1, i);

        int     numPlugins = fP->getPlugins()->count();
        QString numPluginsStr;
        numPluginsStr.sprintf("%d", numPlugins);
        writeUtf8String(out, numPluginsStr);

        for (int j = 0; j < numPlugins; j++) {
            writeUtf8String(out, fP->getPluginFilenames()[j]);
            writeUtf8String(out, fP->getPlugins()->at(j)->getScript());
        }
    }

    displayActivity(tr("Filters saved"));

    file.close();
}

/**
  * Loads the filters from the given file.
  */
void Classifier::loadFilters(QString filename) {
#ifdef _VERBOSE_CLASSIFIER
    qDebug() << "Classifier will load filterset " << filename;
#endif

    // open the file and set up the data stream
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QDataStream in(&file);

    displayActivity(tr("Loading filters"));

    QString numFiltersStr = readUtf8String(in);
    int numFilters = numFiltersStr.toInt();

    // load filters
    for (int i = 0; i < numFilters; i++) {
        QString directory = readUtf8String(in);
        QString parentVirDirPath = readUtf8String(in);
        QString virDirPath = readUtf8String(in);
        bool    recursive = readUtf8String(in) == "TRUE";

        displayActivity(tr("Loading %1").arg(virDirPath));
        displayProgress(0, numFilters - 1, i);

        QString numPluginsStr = readUtf8String(in);
        int numPlugins = numPluginsStr.toInt();

        QStringList pluginNames;
        QStringList pluginScripts;
        for (int j = 0; j < numPlugins; j++) {
            QString pluginName = readUtf8String(in);
            QString pluginScript = readUtf8String(in);

            pluginNames.append(pluginName);
            pluginScripts.append(pluginScript);
        }

        // create the filter

        // find the parent if it was set (it works because filters are created and stored sequentially in the classifier)
        // find parent
        Filter *parentP = parentVirDirPath.isEmpty() ? NULL : findFilter(parentVirDirPath);

        // create (non started) filter
        addFilter(parentP, virDirPath, directory, recursive, pluginNames, true);

        // retrieve newly created filter
        Filter *newFilterP = findFilter(virDirPath);
        if (newFilterP) {
            QVector<PluginInterface *> *pluginsP = newFilterP->getPlugins();
            for (int j = 0; pluginsP && pluginsP->count() == numPlugins && j < numPlugins; j++) {
                // set the plugins' scripts
                newFilterP->setScript(pluginsP->at(j)->getName(), pluginScripts[j]);
            }
        }
    }

    displayActivity(tr("Filters loaded"));

    file.close();
}

/**
  * Saves the database content to filename.
  */
void Classifier::saveDatabase(QString filename) {
    ServerDatabase db;

#ifdef _VERBOSE_CLASSIFIER
    qDebug() << "Classifier will save database " << filename;
#endif

    QFile file(filename);

    // open the file and set up the data stream
    if (!file.open(QIODevice::ReadWrite))
        return;

    QDataStream out(&file);

    displayActivity(tr("Saving database"));

    // get the filters
    QStringList filters = db.getFilters();

    // save the filters
    int     numFilters = filters.count();
    QString numFiltersStr;
    numFiltersStr.sprintf("%d", numFilters);
    writeUtf8String(out, numFiltersStr);

    int numFilter = 0;
    for (QStringList::iterator i = filters.begin(); i != filters.end(); i++) {
        QString filter = *i;
        writeUtf8String(out, filter);
        QString filterId = db.getFilterId(filter);

        displayActivity(tr("Saving filter %1").arg(filter));
        displayProgress(0, numFilters - 1, numFilter++);

        // get the files
        QStringList files = db.getFiles(filterId);

        // save the files
        int     numFiles = files.count();
        QString numFilesStr;
        numFilesStr.sprintf("%d", numFiles);
        writeUtf8String(out, numFilesStr);

        int numFile = 0;
        for (QStringList::iterator j = files.begin(); j != files.end(); j++) {
            QString file = *j;
            writeUtf8String(out, file);

            displayActivity(tr("Saving file %1").arg(file));
            displayProgress(0, numFiles - 1, numFile++);

            // get the attributes
            QStringList attributes = db.getFileAttributes(filterId, file);

            // save the attributes
            int     numAttrs = attributes.count();
            QString numAttrsStr;
            numAttrsStr.sprintf("%d", numAttrs);
            writeUtf8String(out, numAttrsStr);

            for (QStringList::iterator k = attributes.begin(); k != attributes.end(); k++) {
                QString attribute = *k;

                // saves the attribute and its value
                writeUtf8String(out, attribute);
                writeUtf8String(out, db.getFileAttribute(filterId, file, attribute));
            }
        }
    }

    displayActivity(tr("Database saved"));

    file.close();
}

/**
  * Loads the database from filename.
  */
void Classifier::loadDatabase(QString filename) {
    ServerDatabase db;

#ifdef _VERBOSE_CLASSIFIER
    qDebug() << "Classifier will load database " << filename;
#endif

    // open the file and set up the data stream
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QDataStream in(&file);

    displayActivity(tr("Loading database"));

    // load filters
    QString numFiltersStr = readUtf8String(in);
    int numFilters = numFiltersStr.toInt();

    for (int i = 0; i < numFilters; i++) {
        QString filter = readUtf8String(in);
        db.addFilter(filter);

        displayActivity(tr("Loading filter %1").arg(filter));
        displayProgress(0, numFilters - 1, i);

        QString filterId = db.getFilterId(filter);

        // load files
        QString numFilesStr = readUtf8String(in);
        int numFiles = numFilesStr.toInt();

        for (int j = 0; j < numFiles; j++) {
            QString file = readUtf8String(in);

            // if the file doesn't exist anymore, skip it
            // note: we're not checking remote content here because
            // this would be too slow
            if (!QFileInfo(file).exists())
                continue;

            QString fileId = db.addFile(filterId, file);

            displayActivity(tr("Loading file %1").arg(file));
            displayProgress(0, numFiles - 1, j);

            // load attributes
            QString numAttrsStr = readUtf8String(in);
            int numAttrs = numAttrsStr.toInt();

            for (int k = 0; k < numAttrs; k++) {
                QString attribute = readUtf8String(in);
                QString value = readUtf8String(in);
                db.addFileAttribute(fileId, attribute, value);
            }
        }
    }

    displayActivity(tr("Database loaded"));

    file.close();
}

