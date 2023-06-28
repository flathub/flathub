/*************************************************************************
 *                                                                       *
 * ODER's Utilities Library. Copyright (C) 2008 Oleh Derevenko.          *
 * All rights reserved.  e-mail: odar@eleks.com (change all "a" to "e")  *
 *                                                                       *
 * This library is free software; you can redistribute it and/or         *
 * modify it under the terms of EITHER:                                  *
 *   (1) The GNU Lesser General Public License as published by the Free  *
 *       Software Foundation; either version 3 of the License, or (at    *
 *       your option) any later version. The text of the GNU Lesser      *
 *       General Public License is included with this library in the     *
 *       file LICENSE-LESSER.TXT. Since LGPL is the extension of GPL     *
 *       the text of GNU General Public License is also provided for     *
 *       your information in file LICENSE.TXT.                           *
 *   (2) The BSD-style license that is included with this library in     *
 *       the file LICENSE-BSD.TXT.                                       *
 *   (3) The zlib/libpng license that is included with this library in   *
 *       the file LICENSE-ZLIB.TXT                                       *
 *                                                                       *
 * This library is distributed WITHOUT ANY WARRANTY, including implied   *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.      *
 * See the files LICENSE.TXT and LICENSE-LESSER.TXT or LICENSE-BSD.TXT   *
 * or LICENSE-ZLIB.TXT for more details.                                 *
 *                                                                       *
 *************************************************************************/

#ifndef __OU_FLAGSDEFINES_H_INCLUDED
#define __OU_FLAGSDEFINES_H_INCLUDED


#define OU_FLAGS_ENUMFLAGS_MASK(Type, StartingFlag, EnumMax) ((Type)((Type)((Type)((Type)(StartingFlag) << ((EnumMax) - 1)) - (Type)(StartingFlag)) | (Type)((Type)(StartingFlag) << ((EnumMax) - 1))))
#define OU_FLAGS_ENUMFLAGS_START_VALID(Type, StartingFlag, EnumMax) ((Type)((Type)(StartingFlag) << ((EnumMax) - 1)) != 0)
#define OU_FLAGS_STOREENUM_VALUE_IN_MASK(Type, EnumValue, ValueMask) ((Type)(ValueMask) != 0 && ((Type)(EnumValue) & (Type)(~((Type)(ValueMask)))) == 0)
#define OU_FLAGS_FLAG_IS_SINGLE(Type, Flag) ((Type)(Flag) != 0 && ((Type)(Flag) & (Type)((Type)(Flag) - (Type)1)) == 0)


#endif // #ifndef __OU_FLAGSDEFINES_H_INCLUDED
