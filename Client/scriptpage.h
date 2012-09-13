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

#ifndef SCRIPTPAGE_H
#define SCRIPTPAGE_H

#include <QFrame>

#include "treenode.h"
#include "attributecondition.h"

#define SCRIPT_AUTO_SECTION_START_TAG "//#### GENERATED CODE SECTION START - DO NOT MODIFY - DO NOT MOVE ####"
#define SCRIPT_AUTO_SECTION_END_TAG   "//#### GENERATED CODE SECTION END - YOU CAN WRITE CODE BELOW (just keep the manualScript function) ####"

#define ALL_CONDITIONS_TAG          "//#### MATCH ALL"

#define SCRIPT_DEFINITION_START_TAG "//#### SCRIPT DEFINITION - START"
#define SCRIPT_DEFINITION_END_TAG   "//#### SCRIPT DEFINITION - END"

#define ALL_CONDITIONS              tr("All")
#define ANY_CONDITION               tr("Any")

#define MANUAL_PAGE_INDEX           0
#define ASSISTED_PAGE_INDEX         1

namespace Ui {
    class ScriptPage;
}

/**
  * Instances of this class hold the selected filter's plugin script and last error.
  */
class ScriptPage : public QFrame
{
    Q_OBJECT

public:
    explicit ScriptPage(FilterTreeNode *filterP, QString plugin, QString script, QWidget *parent = 0);
    ~ScriptPage();

    QString getScript();
    void    setScript(QString script);
    void    setScriptError(QString error);

    inline QString getFilter() {return m_filter;}
    inline QString getPlugin() {return m_plugin;}

signals:
    void cancelScript();
    void applyScript();

private slots:
    void emitCancelScript() {cancelScript();}
    void emitApplyScript() {applyScript();}
    void on_addConditionButton_clicked();

private:
    Ui::ScriptPage              *ui;
    QString                     m_plugin;
    QString                     m_filter;
    FilterTreeNode              *m_filterP;
    QList<AttributeCondition *> m_conditions;
    QString                     m_manualScriptPart;
    bool                        m_allConditions;

    void analyzeScriptAndBuildUI(QString script);
};

#endif // SCRIPTPAGE_H
