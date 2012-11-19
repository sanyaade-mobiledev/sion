/*
 * SION! Server IMDBApi.com file plugin.
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
#include <QtCore/qplugin.h>
#include <QFile>
#include <QDir>
#include <QPluginLoader>
#include <qdom.h>

#include <qfileext.h>

#include "imdbplugin.h"
#include "scriptrunner.h"

QMap<QString, AttributeCacheEntry *> ImdbPlugin::m_attributesCache;

PluginInterface *ImdbPlugin::newInstance(QString virtualDirectoryPath) {
    ImdbPlugin *newInstanceP = new ImdbPlugin();
    newInstanceP->initialize(virtualDirectoryPath);
    return newInstanceP;
}

void ImdbPlugin::initialize(QString virtualDirectoryPath) {
    FilePlugin::initialize(virtualDirectoryPath);

    // this one keeps only video files
    m_scriptP = new Script("{\n\ttype = plugin.getAttributeValue(\"Type\").toLowerCase();\
\n\tplugin.setResult(\
\n\t\ttype == \"mp4\" ||\
\n\t\ttype == \"mpeg4\" ||\
\n\t\ttype == \"mpg\" ||\
\n\t\ttype == \"avi\" ||\
\n\t\ttype == \"divx\" ||\
\n\t\ttype == \"wmv\" ||\
\n\t\ttype == \"mov\" ||\
\n\t\ttype == \"mkv\");\n}");

    m_attributes.insert(TITLE_ATTR, new Attribute(TITLE_ATTR, tr("Title of the movie"), "String"));
    m_attributes.insert(YEAR_ATTR,  new Attribute(YEAR_ATTR, tr("Year of the movie (in the format yyyy)"), "String"));
    m_attributes.insert(RATED_ATTR,  new Attribute(RATED_ATTR, tr("MPAA Rating"), "String"));
    m_attributes.insert(RELEASED_ATTR,  new Attribute(RELEASED_ATTR, tr("Release date of the movie"), "String"));
    m_attributes.insert(RUNTIME_ATTR,  new Attribute(RUNTIME_ATTR, tr("Duration of the movie"), "String"));
    m_attributes.insert(GENRE_ATTR, new Attribute(GENRE_ATTR, tr("Movie genre (ie: Drama, Action, etc.)"), "String"));
    m_attributes.insert(DIRECTOR_ATTR,  new Attribute(DIRECTOR_ATTR, tr("Movie Director"), "String"));
    m_attributes.insert(WRITER_ATTR,  new Attribute(WRITER_ATTR, tr("Written by"), "String"));
    m_attributes.insert(ACTORS_ATTR,  new Attribute(ACTORS_ATTR, tr("Actor list"), "String"));
    m_attributes.insert(PLOT_ATTR,  new Attribute(PLOT_ATTR, tr("Movie synopsis"), "String"));
    m_attributes.insert(POSTER_ATTR,  new Attribute(POSTER_ATTR, tr("Movie poster url"), "String"));
}

void ImdbPlugin::loadAttributes(QString filepath) {
    QDomElement  root;
    QDomNode     node;
    QString      movieName;
    QString      movieXmlFile;
    QDomDocument xml;

#ifdef _VERBOSE_IMDB_PLUGIN
    qDebug() << "loading attributes for file: " << filepath;
#endif

    // loads the base attributes
    FilePlugin::loadAttributes(filepath);

    // are the attributes in the cache?
    if (retrieveAttributesFromCache(filepath, ImdbPlugin::m_attributesCache))
        return;

    setAttributeValue(TITLE_ATTR, QVariant(tr("Unknown")));
    setAttributeValue(YEAR_ATTR, QVariant(tr("Unknown")));
    setAttributeValue(RATED_ATTR, QVariant(tr("Unknown")));
    setAttributeValue(RELEASED_ATTR, QVariant(tr("Unknown")));
    setAttributeValue(RUNTIME_ATTR, QVariant(tr("Unknown")));
    setAttributeValue(GENRE_ATTR, QVariant(tr("Unknown")));
    setAttributeValue(DIRECTOR_ATTR, QVariant(tr("Unknown")));
    setAttributeValue(WRITER_ATTR, QVariant(tr("Unknown")));
    setAttributeValue(ACTORS_ATTR, QVariant(tr("Unknown")));
    setAttributeValue(PLOT_ATTR, QVariant(tr("Unknown")));
    setAttributeValue(POSTER_ATTR, QVariant(tr("Unknown")));

    // only video files are handled
    QString type = getAttributeValue(TYPE_ATTR).toString().toLower();
    if (type != "mp4" &&
        type != "mpeg4" &&
        type != "mpg" &&
        type != "avi" &&
        type != "divx" &&
        type != "wmv" &&
        type != "mov" &&
        type != "mkv")
        return;

    // loads the movie attributes
    movieName = getAttributeValue(NAME_ATTR).toString();
    movieName = movieName.left(movieName.lastIndexOf("."));
    movieXmlFile = movieName + ".xml";

    // http request
    QFileExt::downloadPage("www.imdbapi.com", "/?t=" + QUrl::toPercentEncoding(movieName) + "&r=XML", movieXmlFile);

    // get attributes from temporary xml file
    // load dom
    QFile file(movieXmlFile);
    if (file.size() == 0) {
        // the site didn't know much about the file, no need to
        // analyze the result.
        file.remove();
        goto endLoadAttributes;
    }

    if (!file.open(QIODevice::ReadOnly))
      goto endLoadAttributes;

    // set dom content
    if(!xml.setContent(&file))
        goto cleanup;

    // get root
    root = xml.documentElement();

    // get attributes
    node = root.firstChild();
    while (!node.isNull()) {
        QDomElement e = node.toElement();
        if (!e.isNull()) {
            if (e.tagName() == "movie") {
                setAttributeValue(TITLE_ATTR, e.attribute("title", tr("Unknown")));
                setAttributeValue(YEAR_ATTR, e.attribute("year", tr("Unknown")));
                setAttributeValue(RATED_ATTR, e.attribute("rated", tr("Unknown")));
                setAttributeValue(RELEASED_ATTR, e.attribute("released", tr("Unknown")));
                setAttributeValue(RUNTIME_ATTR, e.attribute("runtime", tr("Unknown")));
                setAttributeValue(GENRE_ATTR, e.attribute("genre", tr("Unknown")));
                setAttributeValue(DIRECTOR_ATTR, e.attribute("director", tr("Unknown")));
                setAttributeValue(WRITER_ATTR, e.attribute("writer", tr("Unknown")));
                setAttributeValue(ACTORS_ATTR, e.attribute("actors", tr("Unknown")));
                setAttributeValue(PLOT_ATTR, e.attribute("plot", tr("Unknown")));
                setAttributeValue(POSTER_ATTR, e.attribute("poster", tr("Unknown")));
            }
        }

        node = node.nextSibling();
    }

cleanup:
    file.close();

    // remove xml file
    file.remove();

endLoadAttributes:
    // save attributes in the cache
    saveAttributesInCache(filepath, ImdbPlugin::m_attributesCache);
}

Q_EXPORT_PLUGIN2(ImdbPlugin, ImdbPlugin)

