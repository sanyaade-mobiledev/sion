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

#ifndef MP3PLUGIN_H
#define MP3PLUGIN_H

#include "Mp3Plugin_global.h"

#include <QMap>
#include <QString>

#include "fileplugin.h"

#define  MP3_PLUGIN_NAME  "Mp3 File Plugin"
#define  MP3_PLUGIN_TIP   "Handles Basic Files Attributes and Mp3 Tags"

#define TITLE_ATTR      "Title"
#define ARTIST_ATTR     "Artist"
#define ALBUM_ATTR      "Album"
#define YEAR_ATTR       "Year"
#define COMMENT_ATTR    "Comment"
#define TRACK_ATTR      "Track"
#define GENRE_ATTR      "Genre"

//#define _VERBOSE_MP3_PLUGIN 1

class MP3PLUGINSHARED_EXPORT Mp3Plugin : public FilePlugin {
//    Q_OBJECT
    Q_INTERFACES(PluginInterface)

public:
    explicit Mp3Plugin() : FilePlugin() {}

    // there's no way to specify a constructor in a plugin interface (nor a static factory)
    // so we call pluginP = pluginP->newInstance(<vPath>); then unload the plugin.
    PluginInterface *newInstance(QString virtualDirectoryPath);
    void            initialize(QString virtualDirectoryPath);

    /**
     * plugin information
     */
    QString getName() {
        return MP3_PLUGIN_NAME;
    }

    QString getTip() {
        return MP3_PLUGIN_TIP;
    }

    void loadAttributes(QString filepath);

private:
    static  QMap<QString, AttributeCacheEntry *> m_attributesCache; // the attributes cache

    QVariant getMp3TagFromFile(struct id3_file *fileP, const char *tagNameP);
};


#endif // MP3PLUGIN_H
