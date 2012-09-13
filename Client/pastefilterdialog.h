/*
 * SION! Client interacts with SION! Server to let you virtually
 * organize content as you wish on your computer.
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

#ifndef PASTEFILTERDIALOG_H
#define PASTEFILTERDIALOG_H

#include "newfilterdialog.h"
#include "treenode.h"
#include "serverproxy.h"

/**
  * Specializes the NewFilterDialog by changing the title and a couple editable/uneditable widget states.
  */
class PasteFilterDialog : public NewFilterDialog
{
public:
    PasteFilterDialog(ServerProxy *serverP, TreeNode *rootP, QString parentPath, QString filterName, QString directory, QStringList *pluginsP, QWidget *parentP);
};

#endif // PASTEFILTERDIALOG_H
