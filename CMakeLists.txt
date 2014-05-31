# Copyright (c) 2012-2014 Samsung Electronics Co., Ltd All Rights Reserved
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#
#
# @file        CMakeLists.txt
# @author      Dongyeol Lee (dy3.lee@samsung.com)
# @version     0.1
# @brief

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(tts)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "${PREFIX}")
SET(LIBDIR "${PREFIX}/lib")
SET(INCLUDEDIR "${PREFIX}/include")
SET(VERSION 0.2.36)

## Include common directory ##
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/common")

## Dependent packages ##
INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED  
	aul capi-media-audio-io dbus-1 dlog ecore glib-2.0 libxml-2.0 
	mm-session vconf
)

pkg_check_modules(pkgs_test REQUIRED
    ecore
)

## API ##
ADD_SUBDIRECTORY(include)

## Client library ##
ADD_SUBDIRECTORY(client)

## Server daemon ##
ADD_SUBDIRECTORY(server)

## Smack rule ##
IF(${CMAKE_BUILD_PROFILE} STREQUAL "wearable")
    SET(SMACK_RULE_PATH "/etc/smack/accesses2.d")
ELSE()
    SET(SMACK_RULE_PATH "/etc/smack/accesses.d")
ENDIF()

INSTALL(FILES ${CMAKE_SOURCE_DIR}/tts-server.rule DESTINATION ${SMACK_RULE_PATH})

## config ##
INSTALL(FILES ${CMAKE_SOURCE_DIR}/tts-config.xml DESTINATION /usr/lib/voice/tts/1.0)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/sysinfo-tts.xml DESTINATION /etc/config)

## Sample application ##
ADD_SUBDIRECTORY(test)
