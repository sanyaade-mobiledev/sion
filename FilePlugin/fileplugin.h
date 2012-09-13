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

#ifndef FILEPLUGIN_H
#define FILEPLUGIN_H

#include <QMap>
#include <QString>

#include "FilePlugin_global.h"

#include "plugininterface.h"
#include "plugininterfacewrapper.h"
#include "attribute.h"
#include "script.h"
#include "scriptrunner.h"

//#define _VERBOSE_PLUGIN 1

#define  FILE_PLUGIN_NAME  "Base File Plugin"
#define  FILE_PLUGIN_TIP   "Handles Basic File Attributes (creation time, name, path, size, etc)."

#define PATH_ATTR       "Path"
#define NAME_ATTR       "Name"
#define TYPE_ATTR       "Type"
#define SIZE_ATTR       "Size"
#define CREATED_ATTR    "Created"
#define MODIFIED_ATTR   "Modified"
#define READ_ATTR       "Read"
#define LINK_ATTR       "Link"

class FILEPLUGINSHARED_EXPORT FilePlugin : public PluginInterface {
//    Q_OBJECT
    Q_INTERFACES(PluginInterface)

public:
    explicit FilePlugin() : PluginInterface(), m_wrapper(this) {}

    // there's no way to specify a constructor in a plugin interface (nor a static factory)
    // so we call pluginP = pluginP->newInstance(<vPath>); then unload the plugin.
    PluginInterface *newInstance(QString virtualDirectoryPath);
    void            initialize(QString virtualDirectoryPath);

    virtual ~FilePlugin() {
#ifdef _VERBOSE_PLUGIN
        qDebug() << "Deleting Base plugin";
#endif

        // delete script
        if (m_scriptP)
            delete m_scriptP;

#ifdef _VERBOSE_PLUGIN
        qDebug() << "Base plugin clears m_attributes";
#endif
        // delete attributes
        qDeleteAll(m_attributes);
        m_attributes.clear();
    }

    /**
     * plugin information
     */
    QString getName() {
        return FILE_PLUGIN_NAME;
    }

    QString getTip() {
        return FILE_PLUGIN_TIP;
    }

    /**
     * javascript rules getters/setters
     */

    /**
     * a scripted rule is a free well formatted javascript expression using the
     * plugin' attributes (loaded from the currenlty visited file) to filer
     * the file.
     *
     */

    void setScript(QString script) {
        if (m_scriptP)
            m_scriptP->setScript(script);
    }

    QString getScript() {
        if (m_scriptP)
            return m_scriptP->getScript();

        return "";
    }

    QString getScriptLastError() {
        if (m_scriptP)
            return m_scriptP->getLastError();

        return "";
    }


    bool runScript();

    bool checkFile(QString filepath);

    void forceLoadAttributes(QString filepath);
    void loadAttributes(QString filepath);

    /**
     * attribute inspection (easier than reflection huh?)
     */
    const QList<QString> getAttributeNames();

    QString getAttributeClassName(QString attributeName);
    QString getAttributeTip(QString attributeName);

    void setResult(bool value) {
        m_result = value;
    }

    bool getResult() {
        return m_result;
    }

    void        setAttributeValue(QString attributeName, QVariant value);
    QVariant    getAttributeValue(QString attributeName);

    bool        contains(QString regExp);

    PluginInterfaceWrapper *getWrapper() {
        return &m_wrapper;
    }

protected:
    QString                     m_lastLoadedFile;           // if same encountered since lastLoadAttributes, don't load them twice
    QMap<QString, Attribute*>   m_attributes;               // file attributes used to filter files
    QString                     m_virtualDirectoryPath;     // the path of the associated virtual directory in the browser
    Script                      *m_scriptP;                 // javascript rule used to filter files
    bool                        m_result;                   // result of the last run javascript rule
    ScriptRunner                m_scripter;
    PluginInterfaceWrapper      m_wrapper;                  // wraps this to make it available in the script context
};

#endif // FILEPLUGIN_H
