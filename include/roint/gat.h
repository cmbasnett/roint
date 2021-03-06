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
#ifndef __ROINT_GAT_H
#define __ROINT_GAT_H

#ifdef ROINT_INTERNAL
#	include "config.h"
#elif !defined(WITHOUT_ROINT_CONFIG)
#	include "roint/config.h"
#endif

#ifndef ROINT_DLLAPI
#	define ROINT_DLLAPI
#endif
struct ROGrfFile; // forward declaration

#ifdef __cplusplus
extern "C" {
#endif 


#pragma pack(push,1)
/// Ground Cell.
struct ROGatCell {
	/// Height of the cell corners.
	///  ordering : west->east, south->north
	///  value : lower numbers mean higher ground
	///  zoom : 5 units is equivalent to the side of the cell
	float height[4];
	/// Type of cell.
	///  0 - walkable block
	///  1 - non-walkable block
	///  2 - non-walkable water (not snipable)
	///  3 - walkable water
	///  4 - non-walkable water (snipable)
	///  5 - cliff (snipable)
	///  6 - cliff (not snipable)
	///  other - unknown
	int type;
};

/// Ground Altitude File.
/// Supported versions: v1.2
struct ROGat {
	unsigned char vermajor; //< major version
	unsigned char verminor; //< minor version
	unsigned int width;
	unsigned int height;
	struct ROGatCell *cells; //< west->east, south->north ordering
};
#pragma pack(pop)


/// Inspects the gat data and returns the first compatible version. (0 if invalid)
ROINT_DLLAPI unsigned short gat_inspect(const struct ROGat *gat);
/// Loads the gat from a data buffer. (NULL on error)
ROINT_DLLAPI struct ROGat *gat_loadFromData(const unsigned char *data, unsigned long len);
/// Loads the gat from a system file. (NULL on error)
ROINT_DLLAPI struct ROGat *gat_loadFromFile(const char *fn);
/// Loads the gat from a ROGrf file. (NULL on error)
ROINT_DLLAPI struct ROGat *gat_loadFromGrf(struct ROGrfFile*);
/// Saves the gat to a data buffer. Discards incompatible information. (0 on success)
/// WARNING : the 'data_out' data has to be released with the roint free function
ROINT_DLLAPI int gat_saveToData(const struct ROGat *gat, unsigned char **data_out, unsigned long *size_out);
/// Saves the gat to a system file. Discards incompatible information. (0 on success)
ROINT_DLLAPI int gat_saveToFile(const struct ROGat *gat, const char *fn);
/// Frees everything inside the ROGat structure allocated by us (including the gat itself!)
ROINT_DLLAPI void gat_unload(struct ROGat*);


#ifdef __cplusplus
}
#endif 

#endif /* __ROINT_GAT_H */
