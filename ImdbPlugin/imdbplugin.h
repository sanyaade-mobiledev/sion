/*
 * SION! Server mp3 file plugin.
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

#ifndef IMDBPLUGIN_H
#define IMDBPLUGIN_H

#include "ImdbPlugin_global.h"

#include <QMap>
#include <QString>

#include "fileplugin.h"

#define  IMDB_PLUGIN_NAME  "IMDB File Plugin"
#define  IMDB_PLUGIN_TIP   "Retrieves Movie information from video filenames"

#define TITLE_ATTR          "Title"
#define YEAR_ATTR           "Year"
#define RATED_ATTR          "Rated"
#define RELEASED_ATTR       "Released"
#define RUNTIME_ATTR        "Duration"
#define GENRE_ATTR          "Genre"
#define DIRECTOR_ATTR       "Director"
#define WRITER_ATTR         "Writer"
#define ACTORS_ATTR         "Actors"
#define PLOT_ATTR           "Synopsis"
#define POSTER_ATTR         "Poster"
/*
    There are other meta-data which can be extracted from IMDBApi.com, check out
    "http://www.imdbapi.com/?t=<your favorite movie title here>&r=xml" for
    additional meta-data.
*/

//#define _VERBOSE_PLUGIN 1

class IMDBPLUGINSHARED_EXPORT ImdbPlugin : public FilePlugin {
//    Q_OBJECT
    Q_INTERFACES(PluginInterface)

public:
    explicit ImdbPlugin() : FilePlugin() {}

    // there's no way to specify a constructor in a plugin interface (nor a static factory)
    // so we call pluginP = pluginP->newInstance(<vPath>); then unload the plugin.
    PluginInterface *newInstance(QString virtualDirectoryPath);
    void            initialize(QString virtualDirectoryPath);

    /**
     * plugin information
     */
    QString getName() {
        return IMDB_PLUGIN_NAME;
    }

    QString getTip() {
        return IMDB_PLUGIN_TIP;
    }

    void loadAttributes(QString filepath);
};


#endif // IMDBPLUGIN_H
