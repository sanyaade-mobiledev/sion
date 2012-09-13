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

#ifndef PLUGININTERFACEWRAPPER_H
#define PLUGININTERFACEWRAPPER_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QDebug>

#include "plugininterface.h"

/**
  * An instance of this wrapper embbeds every plugin instance to make
  * it visible in the javascript engine execution context.
  */
class PluginInterfaceWrapper : public QObject {
    Q_OBJECT

public:
    // copy stuff
    PluginInterfaceWrapper() : QObject() {m_wrappedPluginP = NULL;}
    ~PluginInterfaceWrapper() {}
    PluginInterfaceWrapper(const PluginInterfaceWrapper &wrapper) : QObject() {
        m_wrappedPluginP = wrapper.m_wrappedPluginP;
    }

    explicit PluginInterfaceWrapper(PluginInterface *pluginP) {
        m_wrappedPluginP = pluginP;
    }

public slots:
    // just reroute to the wrapped object's methods
    void setResult(bool value) {
        if (!m_wrappedPluginP) {
            qDebug() << QObject::tr("pluginInterfaceWrapper::setResult called without plugin wrapped (nop)!!!");
            return;
        }
        m_wrappedPluginP->setResult(value);
    }

    bool getResult() {
        if (!m_wrappedPluginP) {
            qDebug() << QObject::tr("pluginInterfaceWrapper::getResult called without plugin wrapped (returns false)!!!");
            return false;
        }
        return m_wrappedPluginP->getResult();
    }

    QVariant getAttributeValue(QString attributeName) {
        if (!m_wrappedPluginP) {
            qDebug() << QObject::tr("pluginInterfaceWrapper::getAttributeValue(") << attributeName << QObject::tr(") called without plugin wrapped (returns QVariant())!!!");
            return QVariant();
        }
        return m_wrappedPluginP->getAttributeValue(attributeName);
    }

    bool contains(QString regExp) {
        if (!m_wrappedPluginP) {
            qDebug() << QObject::tr("pluginInterfaceWrapper::contains(") << regExp << QObject::tr(") called without plugin wrapped (returns QVariant())!!!");
            return false;
        }
        return m_wrappedPluginP->contains(regExp);
    }

private:
    PluginInterface *m_wrappedPluginP;
};

Q_DECLARE_METATYPE(PluginInterfaceWrapper)

#endif // PLUGININTERFACEWRAPPER_H
