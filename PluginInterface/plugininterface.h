/*
 * SION! Server file plugin interface.
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

#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QList>

#include "PluginInterface_global.h"

// #define _VERBOSE_PLUGIN_INTERFACE 1

/**
 * The plugin interface provides the generic interface required to
 * load/display information/run plugins. This is a pure virtual base class
 * that must be used as a base class when creating a SION! meta-data extractor
 * plugin (see FilePlugin).
 */

class PluginInterfaceWrapper;
class PluginInterface : public QObject {
    Q_OBJECT

public:
    PluginInterface();
    virtual ~PluginInterface();

    // there's no way to specify a constructor in a plugin interface (nor a static factory)
    // so we call pluginP = loader->instance() then pluginP = pluginP->newInstance(<vPath>).
    virtual PluginInterface         *newInstance(QString virtualDirectoryPath) = 0;
    virtual void                    initialize(QString virtualDirectoryPath) = 0;
    virtual QString                 getName()  = 0;
    virtual QString                 getTip() = 0;
    virtual void                    loadAttributes(QString filepath) = 0;
    virtual void                    setScript(QString script)  = 0;
    virtual QString                 getScript()  = 0;
    virtual QString                 getScriptLastError() = 0;
    virtual bool                    runScript() = 0;
    virtual bool                    checkFile(QString filepath) = 0;
    virtual const QList<QString>    getAttributeNames() = 0;
    virtual QString                 getAttributeClassName(QString attributeName) = 0;
    virtual QString                 getAttributeTip(QString attributeName) = 0;
    virtual void                    setResult(bool value) = 0;
    virtual bool                    getResult() = 0;
    virtual void                    setAttributeValue(QString attributeName, QVariant value) = 0;
    virtual QVariant                getAttributeValue(QString attributeName) = 0;
    virtual bool                    contains(QString regExp) = 0;
    virtual PluginInterfaceWrapper  *getWrapper() = 0;
};

#endif // PLUGININTERFACE_H
