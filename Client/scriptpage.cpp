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

#include "ui_scriptpage.h"
#include "scriptpage.h"
#include "attributecondition.h"

/**
  * Constructs a new script page to be embedded in the browser's plugins' scripts book.
  */
ScriptPage::ScriptPage(FilterTreeNode *filterP, QString plugin, QString script, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::ScriptPage) {
    ui->setupUi(this);

    ui->conditionTypeCombo->addItem(ALL_CONDITIONS);
    ui->conditionTypeCombo->addItem(ANY_CONDITION);
    ui->conditionTypeCombo->setCurrentIndex(ui->conditionTypeCombo->findText(ALL_CONDITIONS));
    m_allConditions = true;

    m_filterP = filterP;
    m_filter = filterP->getPath();
    m_plugin = plugin;

    setScript(script);

    // if an assisted script was found, select the assisted scripting page
    // else select the manual scripting page
    if (!m_conditions.empty())
        ui->toolBox->setCurrentIndex(ASSISTED_PAGE_INDEX);
    else
        ui->toolBox->setCurrentIndex(MANUAL_PAGE_INDEX);
}

/**
  * Deletes the script page.
  */
ScriptPage::~ScriptPage() {
    delete ui;
}

/**
  * Sets the current script page content with the given script javascript source code.
  */
void ScriptPage::setScript(QString script) {
    ui->scriptText->clear();
    ui->scriptText->appendPlainText(script);

    // analyzes the script and builds the assisted scripting page
    analyzeScriptAndBuildUI(script);
}

/**
  * Displays the last encountered javascript error for the script.
  */
void ScriptPage::setScriptError(QString error) {
    QString redError = "<font color='red'>" + error + "</font>";
    ui->scriptError->setText(redError);
}

/**
  * Returns the script's javascript source code edited in the page.
  */
QString ScriptPage::getScript() {
    QString script;

    // pick the script from the selected page
    switch (ui->toolBox->currentIndex()) {
        case MANUAL_PAGE_INDEX:
            // manual..
            script = ui->scriptText->toPlainText();
            break;

        case ASSISTED_PAGE_INDEX:
            // get the selected condition type
            m_allConditions = ui->conditionTypeCombo->currentText() == ALL_CONDITIONS;

            // if no manual part, build an empty one
            if (!m_manualScriptPart.contains("manualScript"))
                m_manualScriptPart = "function manualScript(result) {\n\treturn true;\n}";

            // assisted
            script = "{plugin.setResult(false);}";
            int numConditions = m_conditions.count();
            if (numConditions > 0) {
                script = SCRIPT_AUTO_SECTION_START_TAG;
                if (m_allConditions) {
                    script += "\n";
                    script += ALL_CONDITIONS_TAG;
                }
                script += "\n{\n";
                script += "\tresult =\n\t\t";
                script += SCRIPT_DEFINITION_START_TAG;
                for (int i = 0; i < numConditions; i++) {
                    AttributeCondition *conditionP = dynamic_cast<AttributeCondition *>(m_conditions[i]);
                    if (conditionP) {
                        script += "\n\t\t" + conditionP->getDefinition();
                        script += "\n\t\t" + conditionP->getScript();
                        if (i < numConditions - 1) {
                            if (m_allConditions)
                                script += " &&";
                            else
                                script += " ||";
                        }
                    }
                }
                script += ";\n\t\t";
                script += SCRIPT_DEFINITION_END_TAG;
                script += "\n\tresult &= manualScript(result);";
                script += "\n\tplugin.setResult(result);\n}\n";
                script += SCRIPT_AUTO_SECTION_END_TAG;
                script += "\n";
                script += m_manualScriptPart;
            }
            break;
    }

    return script;
}

/**
  * Analyzes the script and dynamically builds the UI based on the analysis result. An assisted script
  * has initially (before the user modifies the manualScript function) the following syntax:
  *
  *     assisted_script :- SCRIPT_AUTO_SECTION_START_TAG "\n" ,
  *                        [ALL_CONDITIONS_TAG "\n"] ,
  *                        "\n{\n" ,
  *                        "\tresult =\n\t\t" ,
  *                        (assisted_script_condition \n)* ,
  *                        SCRIPT_AUTO_SECTION_END_TAG "\n" ,
  *                         ";\n\t\t" ,
  *                        SCRIPT_DEFINITION_END_TAG ,
  *                        "\n\tresult &= manualScript(result);"  ,
  *                        "\n\tplugin.setResult(result);\n}\n" ,
  *                        SCRIPT_AUTO_SECTION_END_TAG "\n" ,
  *                        "function manualScript(result) {\n\treturn true;\n}" ;
  *
  *     assisted_script_condition :- "\n\t\t" condition_definition ("&&"|"||"),
  *                                  "\n\t\t" JAVASCRIPT ;
  *
  *    condition_definition :- "//" ATTRIBUTE_NAME "," OPERATOR "," VALUE
  */
void ScriptPage::analyzeScriptAndBuildUI(QString script) {
    // no conditions before we've analyzed the script
    for (int i = 0; i < m_conditions.count(); i++)
        delete m_conditions[i];
    m_conditions.clear();

    // no manual script either
    m_manualScriptPart.clear();

    // does the script start with the SCRIPT_DEFINITION tag?
    if (script.startsWith(SCRIPT_AUTO_SECTION_START_TAG)) {
        QStringList lines = script.split("\n");
        int numLines = lines.count();

        // skip the first lines
        m_allConditions = false;

        QString line;
        int     i = 0;
        while (i < numLines) {
            QString line = lines[i++];
            // do we use all conditions or any of them?
            if (!m_allConditions)
                m_allConditions = line.contains(ALL_CONDITIONS_TAG);
            ui->conditionTypeCombo->setCurrentIndex(ui->conditionTypeCombo->findText(m_allConditions ? ALL_CONDITIONS : ANY_CONDITION));

            if (line.contains(SCRIPT_DEFINITION_START_TAG))
                break;
        }

        // analyze the definition comments until the END TAG is reached
        while (i < numLines) {
            line = lines[i++];

            // when the end is reached, exit loop
            if (line.contains(SCRIPT_DEFINITION_END_TAG))
                break;

            // get the attribute name, it's right after the "//"
            int offset = line.indexOf("//");
            if (offset == -1)
                    break; // someone has modified the script...

            offset += 2;
            int attrNameEnd = line.indexOf(",", offset);
            if (attrNameEnd == -1)
                break; // someone has modified the script...

            QString attribute = line.mid(offset, attrNameEnd - offset);

            offset = attrNameEnd + 1;
            int operatorEnd = line.indexOf(",", offset);
            if (operatorEnd == -1)
                break; // someone has modified the script...

            QString op = line.mid(offset, operatorEnd - offset);

            offset = operatorEnd + 1;
            int valueEnd = line.indexOf(";", offset);
            if (valueEnd == -1)
                break; // someone has modified the script...

            QString value = line.mid(offset, valueEnd - offset);

            // create an attribute condition for each line until the end tag is met
            AttributeCondition *conditionP = new AttributeCondition(&m_conditions, m_filterP, m_plugin, attribute, op, value);
            ui->conditionsVBox->addWidget(conditionP, 0, Qt::AlignLeft | Qt::AlignBottom);
            m_conditions += conditionP;


            ++i; // skip script line and go to the next definition
        }

        // skip everything until the end of the generated section
        while (i < numLines) {
            line = lines[i++];
            if (line.contains(SCRIPT_AUTO_SECTION_END_TAG))
                break;
        }

        // save the pending lines
        while (i < numLines) {
            m_manualScriptPart += lines[i++];
            m_manualScriptPart += "\n";
        }
    }
}

/**
  * Slot invoked when the 'add condition' button is clicked. Creates a new AttributeCondition widget
  * and insert it into the page.
  */
void ScriptPage::on_addConditionButton_clicked() {
    AttributeCondition *conditionP = new AttributeCondition(&m_conditions, m_filterP, m_plugin);
    ui->conditionsVBox->addWidget(conditionP, 0, Qt::AlignLeft | Qt::AlignBottom);
    m_conditions += conditionP;
}
