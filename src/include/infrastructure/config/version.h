/**
 * @file version.h
 * @brief FWlication version file
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2025 Oryx Embedded SARL. All rights reserved.
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.5.0
 **/

#ifndef _VERSION_H
#define _VERSION_H

//Major version
#define FW_MAJOR_VERSION 1
#define HW_MAJOR_VERSION 5
//Minor version
#define FW_MINOR_VERSION 1
#define HW_MINOR_VERSION 0
//Revision number
#define FW_REV_NUMBER 6
#define HW_REV_NUMBER 0
//Version string
#define FW_VERSION_STRING "1.1.6"
#define HW_VERSION_STRING "5.0.0"
//Version color
#define FW_VERSION_COLOR 0xFFE082
//Versio
#define FW_VERSION (uint32_t)(((FW_MAJOR_VERSION & 0xFF) << 16) | ((FW_MINOR_VERSION & 0xFF) << 8) | (FW_REV_NUMBER & 0xFF))
#define HW_VERSION (uint32_t)(((HW_MAJOR_VERSION & 0xFF) << 16) | ((HW_MINOR_VERSION & 0xFF) << 8) | (HW_REV_NUMBER & 0xFF))
#endif //!_VERSION_H
