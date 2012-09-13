/*
 * SION! Server meta-data / javascript indexing server.
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

#ifndef SERVERCOMMANDS_H
#define SERVERCOMMANDS_H

#define SION_SERVER_EXECUTABLE_NAME        	"SION!Server"
#define SION_SERVER_ORGANIZATION           	"Intel Corporation"

#define PLUGIN_SUFFIX                           "Plugin.so"     // plugin names must end like this (and any file in the server working directory which ends like this is considered a plugin)
#define FILTER_SET_SUFFIX                       "SION!"        	// SION! Filter set file...
#define DB_EXTENSION                            ".DB"           // SION! Filter set Database extension

#define MAX_FILE_CHUNK_SIZE                     10240           // when exchanging files with the server, max chunk size

#define ACCESS_COMMAND                          "ACCESS"
#define EXIT_COMMAND                            "EXIT"
#define HELP_COMMAND                            "HELP"
#define PING_COMMAND                            "PING"
#define FILTERS_COMMAND                         "FILTERS"
#define DIRECTORIES_COMMAND                     "DIRECTORIES"
#define ADD_FILTER_COMMAND                      "ADD_FILTER"
#define START_FILTER_COMMAND                    "START_FILTER"
#define STOP_FILTER_COMMAND                     "STOP_FILTER"
#define IS_FILTER_RUNNING_COMMAND               "IS_FILTER_RUNNING"
#define MODIFY_FILTER_COMMAND                   "MODIFY_FILTER"
#define REMOVE_FILTER_COMMAND                   "REMOVE_FILTER"
#define GET_FILTER_DIR_COMMAND                  "GET_FILTER_DIR"

#define SETS_COMMAND                            "SETS"
#define SET_COMMAND                             "SET"
#define NEW_SET_COMMAND                         "NEW_SET"
#define IS_SET_DIRTY_COMMAND                    "IS_SET_DIRTY"
#define SAVE_SET_COMMAND                        "SAVE_SET"
#define LOAD_SET_COMMAND                        "LOAD_SET"
#define DELETE_SET_COMMAND                      "DELETE_SET"

#define AVAILABLE_PLUGINS_COMMAND               "AVAILABLE_PLUGINS"
#define PLUGINS_COMMAND                         "PLUGINS"
#define GET_PLUGIN_TIP_COMMAND                  "GET_PLUGIN_TIP"
#define SET_PLUGIN_SCRIPT_COMMAND               "SET_PLUGIN_SCRIPT"
#define GET_PLUGIN_SCRIPT_COMMAND               "GET_PLUGIN_SCRIPT"
#define GET_PLUGIN_SCRIPT_LAST_ERROR_COMMAND    "GET_PLUGIN_SCRIPT_LAST_ERROR"

#define ATTRIBUTES_COMMAND                      "ATTRIBUTES"
#define GET_PLUGIN_ATTRIBUTE_CLASS_COMMAND      "GET_PLUGIN_ATTRIBUTE_CLASS"
#define GET_PLUGIN_ATTRIBUTE_TIP_COMMAND        "GET_PLUGIN_ATTRIBUTE_TIP"

#define GET_FILE_ATTRIBUTE_VALUE_COMMAND        "GET_FILE_ATTRIBUTE_VALUE"

#define GET_FILE_COMMAND                        "GET_FILE"

#define FILES_COMMAND                           "FILES"
#define RESCAN_COMMAND                          "RESCAN"
#define RESCAN_FILTER_COMMAND                   "RESCAN_FILTER"
#define SCAN_COMMAND                            "SCAN"
#define CLEANUP_COMMAND                         "CLEANUP"
#define CLEANUP_FILTER_COMMAND                  "CLEANUP_FILTER"

// unexpected messages sent by the server
#define ADD_FILE_MSG                            "ADD_FILE"
#define DEL_FILE_MSG                            "DEL_FILE"

#define ADD_FILTER_MSG                          "ADD_FILTER"
#define DEL_FILTER_MSG                          "DEL_FILTER"

#define ADD_FILTER_SET_MSG                      "ADD_FILTER_SET"
#define DEL_FILTER_SET_MSG                      "DEL_FILTER_SET"

#define REQUEST_ACCESS_GRANT_MSG                "REQUEST_ACCESS_GRANT"

#define CMD_SEPARATOR                           '\x1e'

#endif // SERVERCOMMANDS_H
