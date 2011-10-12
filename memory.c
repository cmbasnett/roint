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
#include "memory.h"

#include <stdlib.h>

roint_alloc_func _xalloc = &malloc;
roint_free_func _xfree = &free;

void set_roint_malloc_func(roint_alloc_func x) {
	if (x == NULL)
		_xalloc = &malloc;
	else
		_xalloc = x;
}

void set_roint_free_func(roint_free_func x) {
	if (x == NULL)
		_xfree = &free;
	else
		_xfree = x;
}

roint_alloc_func get_roint_malloc_func() {
    return(_xalloc);
}

roint_free_func get_roint_free_func() {
    return(_xfree);
}
