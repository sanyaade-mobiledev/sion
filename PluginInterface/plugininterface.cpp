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

#include <QDebug>

#include "plugininterface.h"

PluginInterface::PluginInterface() : QObject() {
#ifdef _VERBOSE_PLUGIN_INTERFACE
        qDebug() << "creating a plugin interface...";
#endif
}

PluginInterface::~PluginInterface() {
#ifdef _VERBOSE_PLUGIN_INTERFACE
    qDebug() << "destroying a plugin interface...";
#endif
}
