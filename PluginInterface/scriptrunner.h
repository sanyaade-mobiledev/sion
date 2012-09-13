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

#ifndef SCRIPTRUNNER_H
#define SCRIPTRUNNER_H

#include <QObject>
#include <QScriptEngine>
#include <QSemaphore>

#include "script.h"
#include "plugininterface.h"

//#define _VERBOSE_PLUGIN_INTERFACE 1

/**
 * Runs the rules script associated with a plugin: publishes the plugin object into the
 * QScriptEngine execution context, executes the script, and sets the plugin's script result.
 */

class PluginInterface;
class ScriptRunner : public QObject
{
    Q_OBJECT

public:
    explicit ScriptRunner(QObject *parentP = 0);
    ~ScriptRunner();

    bool run(Script *scriptP, PluginInterface *pluginP);

signals:

public slots:

private:
    static QScriptEngine *m_scriptEngineP;
    static int           m_scriptEngineRefCount;
    static QSemaphore    *m_runningScriptP;           // don't run script concurrently since we have a single script engine for all filters/watchers...
};

#endif // SCRIPTRUNNER_H
