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

#include <QDebug>
#include <QtCore/qplugin.h>
#include <QFile>
#include <QDir>
#include <QPluginLoader>

#include <id3tag.h>

#include "mp3plugin.h"
#include "scriptrunner.h"

QMap<QString, AttributeCacheEntry *> Mp3Plugin::m_attributesCache;

PluginInterface *Mp3Plugin::newInstance(QString virtualDirectoryPath) {
    Mp3Plugin *newInstanceP = new Mp3Plugin();
    newInstanceP->initialize(virtualDirectoryPath);
    return newInstanceP;
}

void Mp3Plugin::initialize(QString virtualDirectoryPath) {
    FilePlugin::initialize(virtualDirectoryPath);

    // this one keeps only mp3 files
    m_scriptP = new Script("{\n\tplugin.setResult(plugin.getAttributeValue(\"Type\").toLowerCase() == \"mp3\");\n}");

    m_attributes.insert(GENRE_ATTR, new Attribute(GENRE_ATTR, tr("Music genre (ie: rock, pop, etc.)"), "String"));
    m_attributes.insert(ALBUM_ATTR,  new Attribute(ALBUM_ATTR, tr("Album the music comes from"), "String"));
    m_attributes.insert(TITLE_ATTR, new Attribute(TITLE_ATTR, tr("Title of the music"), "String"));
    m_attributes.insert(ARTIST_ATTR,  new Attribute(ARTIST_ATTR, tr("Artist interpretating the music"), "String"));
    m_attributes.insert(YEAR_ATTR,  new Attribute(YEAR_ATTR, tr("Year of the music (in the format yyyy)"), "String"));
    m_attributes.insert(COMMENT_ATTR,  new Attribute(COMMENT_ATTR, tr("A comment about the music"), "String"));
    m_attributes.insert(TRACK_ATTR,  new Attribute(TRACK_ATTR, tr("Track number"), "Numeric"));
}

void Mp3Plugin::loadAttributes(QString filepath) {
#ifdef _VERBOSE_MP3_PLUGIN
    qDebug() << "loading attributes for file: " << filepath;
#endif

    // loads the base attributes
    FilePlugin::loadAttributes(filepath);

    // are the attributes in the cache?
    if (retrieveAttributesFromCache(filepath, Mp3Plugin::m_attributesCache))
        return;

    setAttributeValue(GENRE_ATTR, QVariant(tr("Unknown")));
    setAttributeValue(ALBUM_ATTR, QVariant(tr("Unknown")));
    setAttributeValue(TITLE_ATTR, QVariant(tr("Unknown")));
    setAttributeValue(ARTIST_ATTR, QVariant(tr("Unknown")));
    setAttributeValue(YEAR_ATTR, QVariant(tr("Unknown")));
    setAttributeValue(COMMENT_ATTR, QVariant(tr("Unknown")));
    setAttributeValue(TRACK_ATTR, QVariant(tr("Unknown")));

    // only mp3 files are handled
    if (getAttributeValue(TYPE_ATTR).toString().toLower() != "mp3")
        return;

    // loads the mp3 attributes
    id3_file *fileP = id3_file_open(filepath.toLocal8Bit().data(), ID3_FILE_MODE_READONLY);

    // genre
    QVariant genre = getMp3TagFromFile(fileP, ID3_FRAME_GENRE);
    if (!genre.isNull()) {
        id3_ucs4_t const *genreNameP = id3_genre_name(id3_genre_index(genre.toInt()));
        if (genreNameP) {
            char *stringP = (char *)id3_ucs4_utf8duplicate(genreNameP);
            setAttributeValue(GENRE_ATTR, QVariant(stringP));
            free(stringP);
        }
    }

    // album
    setAttributeValue(ALBUM_ATTR, getMp3TagFromFile(fileP, ID3_FRAME_ALBUM));

    // title
    setAttributeValue(TITLE_ATTR, getMp3TagFromFile(fileP, ID3_FRAME_TITLE));

    // artist
    setAttributeValue(ARTIST_ATTR, getMp3TagFromFile(fileP, ID3_FRAME_ARTIST));

    // year
    setAttributeValue(YEAR_ATTR, getMp3TagFromFile(fileP, ID3_FRAME_YEAR));

    // title
    setAttributeValue(COMMENT_ATTR, getMp3TagFromFile(fileP, ID3_FRAME_COMMENT));

    // track
    setAttributeValue(TRACK_ATTR, getMp3TagFromFile(fileP, ID3_FRAME_TRACK));

    id3_file_close(fileP);

    // save attributes in the cache
    saveAttributesInCache(filepath, Mp3Plugin::m_attributesCache);
}

QVariant Mp3Plugin::getMp3TagFromFile(struct id3_file *fileP, const char *tagNameP) {
    char        *stringP;
    QString     string;
    QVariant    result;
#ifdef _VERBOSE_MP3_PLUGIN
    QString     str;
#endif

    struct id3_tag *tagP = id3_file_tag(fileP);

    struct id3_frame *frameP;
    if ((frameP = id3_tag_findframe(tagP, tagNameP, 0))) {
#ifdef _VERBOSE_MP3_PLUGIN
        str.sprintf("%s (%s):", frameP->id, frameP->description);
        qDebug() << str;
#endif
        for (unsigned j = 0;j < frameP->nfields; ++j) {
            union id3_field *fieldP = frameP->fields + j;

#ifdef _VERBOSE_MP3_PLUGIN
            str.sprintf("field type: %s",(const char *[]){
               "ID3_FIELD_TYPE_TEXTENCODING",
               "ID3_FIELD_TYPE_LATIN1",
               "ID3_FIELD_TYPE_LATIN1FULL",
               "ID3_FIELD_TYPE_LATIN1LIST",
               "ID3_FIELD_TYPE_STRING",
               "ID3_FIELD_TYPE_STRINGFULL",
               "ID3_FIELD_TYPE_STRINGLIST",
               "ID3_FIELD_TYPE_LANGUAGE",
               "ID3_FIELD_TYPE_FRAMEID",
               "ID3_FIELD_TYPE_DATE",
               "ID3_FIELD_TYPE_INT8",
               "ID3_FIELD_TYPE_INT16",
               "ID3_FIELD_TYPE_INT24",
               "ID3_FIELD_TYPE_INT32",
               "ID3_FIELD_TYPE_INT32PLUS",
               "ID3_FIELD_TYPE_BINARYDATA"
             }[fieldP->type]);
             qDebug() << str;
#endif

            switch(fieldP->type) {
                case ID3_FIELD_TYPE_TEXTENCODING:
#ifdef _VERBOSE_MP3_PLUGIN
                    str.sprintf("text encoding: %s",(const char *[]){
                         "ID3_FIELD_TEXTENCODING_ISO_8859_1",
                         "ID3_FIELD_TEXTENCODING_UTF_16",
                         "ID3_FIELD_TEXTENCODING_UTF_16BE",
                         "ID3_FIELD_TEXTENCODING_UTF_8"
                           } [fieldP->number.value]);
                    qDebug() << str;
#endif
                    break;

                case ID3_FIELD_TYPE_INT8:
                case ID3_FIELD_TYPE_INT16:
                case ID3_FIELD_TYPE_INT24:
                case ID3_FIELD_TYPE_INT32:
#ifdef _VERBOSE_MP3_PLUGIN
                    str.sprintf("integer %li", fieldP->number.value);
                    qDebug() << str;
#endif
                    result = QVariant((int)fieldP->number.value);
                    break;

                case ID3_FIELD_TYPE_LATIN1:
                case ID3_FIELD_TYPE_LATIN1FULL:
                    stringP = (char *)fieldP->latin1.ptr;
#ifdef _VERBOSE_MP3_PLUGIN
                    str.sprintf("latin or latin full %s", stringP);
                    qDebug() << str;
#endif
                    result = QVariant(stringP ? stringP : "");
                    break;

                case ID3_FIELD_TYPE_LATIN1LIST:
                    for (unsigned k = 0; k < fieldP->latin1list.nstrings; ++k) {
                        stringP = (char *)fieldP->latin1list.strings[k];

#ifdef _VERBOSE_MP3_PLUGIN
                        str.sprintf("latin string list %u: %s\n", k, stringP);
                        qDebug() << str;
#endif
                        string += QString(stringP ? stringP : "");
                    }
                    result = QVariant(string);
                    break;

                case ID3_FIELD_TYPE_STRING:
                case ID3_FIELD_TYPE_STRINGFULL:
                     if (!fieldP->string.ptr) {
                         result = QVariant("");
                         break;
                     }
                     stringP = (char *)id3_ucs4_latin1duplicate(fieldP->string.ptr);

#ifdef _VERBOSE_MP3_PLUGIN
                    str.sprintf("string or string full: %s", stringP);
                    qDebug() << str;
#endif

                    result = QVariant(stringP ? stringP : "");
                    free(stringP);
                    break;

                case ID3_FIELD_TYPE_STRINGLIST:
                    for(unsigned k = 0; k < fieldP->stringlist.nstrings; ++k) {
                        if (!fieldP->stringlist.strings[k])
                            continue;

                        stringP = (char *)id3_ucs4_latin1duplicate(fieldP->stringlist.strings[k]);
#ifdef _VERBOSE_MP3_PLUGIN
                        str.sprintf("string list %u: %s\n", k, stringP);
                        qDebug() << str;
#endif
                        string += QString(stringP ? stringP : "");
                        free(stringP);
                    }
                    result = QVariant(string);
                    break;

                case ID3_FIELD_TYPE_LANGUAGE:
                case ID3_FIELD_TYPE_FRAMEID:
                case ID3_FIELD_TYPE_DATE:
                    stringP = fieldP->immediate.value;

#ifdef _VERBOSE_MP3_PLUGIN
                    str.sprintf("immediate value: %s", stringP);
                    qDebug() << str;
#endif

                    result = QVariant(stringP ? stringP : "");
                    break;

                case ID3_FIELD_TYPE_INT32PLUS:
                case ID3_FIELD_TYPE_BINARYDATA:
#ifdef _VERBOSE_MP3_PLUGIN
                    str = "binary data: ";
                    for(id3_length_t k = 0; k < fieldP->binary.length;) {
                        QString byteStr;
                        byteStr.sprintf("%02X", fieldP->binary.data[k++]);
                        str.append(byteStr);
                    }
                    qDebug() << str;
#endif
                    result = QVariant(QByteArray((const char *)fieldP->binary.data, fieldP->binary.length));
                    break;

#ifdef _VERBOSE_MP3_PLUGIN
                default:
                    qDebug() << "???";
#endif
            }
        }
    }

    return result;
}

Q_EXPORT_PLUGIN2(Mp3Plugin, Mp3Plugin)

