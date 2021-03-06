/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of The Open Ragnarok Project
    Copyright 2007 - 2011 The Open Ragnarok Team
    For the latest information visit http://www.open-ragnarok.org
    ------------------------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place - Suite 330, Boston, MA 02111-1307, USA, or go to
    http://www.gnu.org/copyleft/lesser.txt.
    ------------------------------------------------------------------------------------
*/

/* 
  WARNING:THIS FILE SHOULD NOT BE UPDATED INDEPENDENTLY.
  It is a "port" of the autoconf header for Visual Studio.
  Please, make the proper changed on the autoconf file if needed.
*/
#ifndef __ROINT_INTERNAL_CONFIG_H
#define __ROINT_INTERNAL_CONFIG_H

//#define HAVE_UNISTD_H
#define HAVE_IO_H

#define HAVE_FTRUNCATE
//#define HAVE_FTRUNCATE64
#define HAVE_CHSIZE
#define HAVE__CHSIZE_S

#ifdef _MSC_VER
#	ifdef ROINT_DLL
#		pragma comment(lib, "libz.dll.a")
#	else
#		pragma comment(lib, "libz.a")
#	endif
#endif

#endif /* __ROINT_CONFIG_H */
