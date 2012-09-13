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

#ifndef CLASSIFIER_H
#define CLASSIFIER_H

#include <QObject>
#include <QVector>
#include <QDebug>

#include "filter.h"

//#define _VERBOSE_CLASSIFIER 1

/**
 * The Classifier holds filters on physical directories. A filter
 * is an instance of a (file) Filter.
 *
 * The Classifier is the interface used by the server to access
 * filters.
 */
class Classifier : public QObject {
    Q_OBJECT

signals:
    void displayActivity(QString string);
    void displayProgress(int min, int max, int value);

public:
    explicit Classifier(QObject *parentP = 0);
    ~Classifier() {
#ifdef _VERBOSE_CLASSIFIER
        qDebug() << "deleting Classifier, will remove all filters";
#endif

        removeAllFilters();
    }

    inline QVector<Filter *> *getFilters() {
        return &m_filters;
    }

    void    removeFilter(Filter *filterP);
    void    removeAllFilters() {
        // remove all filters
        while (m_filters.count())
            removeFilter(m_filters[0]); // not very efficient, removing root filters should be enough.

        // full db cleanup (will reset auto_increment)
        ServerDatabase db;
        db.cleanup(true);
    }

    void    saveFilters(QString filename);
    void    loadFilters(QString filename);

    void    saveDatabase(QString filename);
    void    loadDatabase(QString filename);

    Filter  *addFilter(Filter *parentP, QString virtualDirectoryPath, QString dir, bool recursive, QStringList plugins, bool dontStart = false);
    Filter  *findFilter(QString virtualDirectoryPath);
    void    modifyFilter(Filter *filterP, QString dir, bool recursive, QStringList pluginNames) {
        if (filterP)
            filterP->modifyFilter(dir, recursive, pluginNames);
    }


    void    start();
    void    stop();

    void    rescan();
    void    scan();

signals:
    void newFilter(QString virtualDirectoryPath);
    void delFilter(QString virtualDirectoryPath);

public slots:

private:
    QVector<Filter *> m_filters; // all the filters

    QString readUtf8String(QDataStream &in);
    void    writeUtf8String(QDataStream &out, QString string);
};

#endif // CLASSIFIER_H
