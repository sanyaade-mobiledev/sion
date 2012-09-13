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

#include "scriptrunner.h"
#include "plugininterfacewrapper.h"

#include <QVariant>
#include <QDebug>

QScriptEngine *ScriptRunner::m_scriptEngineP = NULL;
int           ScriptRunner::m_scriptEngineRefCount = 0;
QSemaphore    *ScriptRunner::m_runningScriptP = NULL;

ScriptRunner::ScriptRunner(QObject *parentP) : QObject(parentP) {
    // create the engine if it doesn't exist yet.
    if (++m_scriptEngineRefCount == 1) {
        m_scriptEngineP = new QScriptEngine();
        m_runningScriptP = new QSemaphore(1);
    }
}

ScriptRunner::~ScriptRunner() {
    // a little garbage collection?
    m_scriptEngineP->collectGarbage();

    // destroy the engine if it exists.
    if (--m_scriptEngineRefCount == 0) {
        delete m_scriptEngineP;
        m_scriptEngineP = NULL;
        delete m_runningScriptP;
        m_runningScriptP = NULL;
    }
}

bool ScriptRunner::run(Script *scriptP, PluginInterface *pluginP) {
    bool result = FALSE;

    if (!m_scriptEngineP || !m_runningScriptP)
        return FALSE;

    m_runningScriptP->acquire();

    // push the engine context
    m_scriptEngineP->pushContext();

    // retrieves the script and make the plugin accessible from the javascript (through a wrapper)
    QString script = scriptP->getScript();
    QScriptValue go = m_scriptEngineP->globalObject();
    go.setProperty("plugin", m_scriptEngineP->newQObject(pluginP->getWrapper()));

    // do static check of the script
    if (!m_scriptEngineP->canEvaluate(script)) {
        scriptP->setError(QObject::tr("Script can't be evaluated"));
#ifdef _VERBOSE_PLUGIN_INTERFACE
        qDebug() << scriptP->getLastError();
#endif
        goto engineCleanUp;
    }

    // actually run the script
    m_scriptEngineP->evaluate(script);

    // uncaught exception?
    if (m_scriptEngineP->hasUncaughtException()) {
        QScriptValue exception = m_scriptEngineP->uncaughtException();
        int line = m_scriptEngineP->uncaughtExceptionLineNumber() - 1;
        scriptP->setError(QString("line %1: ").arg(line) + exception.toString());
        m_scriptEngineP->clearExceptions();
#ifdef _VERBOSE_PLUGIN_INTERFACE
        qDebug() << scriptP->getLastError() << " cleared..";
#endif
        goto engineCleanUp;
    }

    result = pluginP->getResult();

engineCleanUp:
    // pop the engine context
    m_scriptEngineP->popContext();

    m_runningScriptP->release();

    return result;
}
