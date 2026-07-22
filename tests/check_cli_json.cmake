# SPDX-License-Identifier: GPL-2.0
# Copyright (C) 2026 Piotr Karbowski

if(CASE STREQUAL "dump-table")
    set(arguments --json --dump-table)
elseif(CASE STREQUAL "repeated-bare")
    set(arguments --json --json)
else()
    set(arguments --json)
endif()

execute_process(
    COMMAND "${PROGRAM}" ${arguments}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr)

if(NOT result EQUAL 2)
    message(FATAL_ERROR "expected exit code 2, got ${result}")
endif()
if(NOT stderr STREQUAL "")
    message(FATAL_ERROR "expected empty stderr, got: ${stderr}")
endif()
if(NOT stdout MATCHES "\"success\": false")
    message(FATAL_ERROR "missing unsuccessful result in stdout: ${stdout}")
endif()
if(NOT stdout MATCHES "\"stage\": \"arguments\"")
    message(FATAL_ERROR "missing argument error in stdout: ${stdout}")
endif()
string(FIND "${stdout}" "\"error\": {" error_position)
if(error_position EQUAL -1)
    message(FATAL_ERROR "missing singular error object in stdout: ${stdout}")
endif()
string(FIND "${stdout}" "\"errors\"" errors_position)
if(NOT errors_position EQUAL -1)
    message(FATAL_ERROR "found obsolete errors array in stdout: ${stdout}")
endif()
