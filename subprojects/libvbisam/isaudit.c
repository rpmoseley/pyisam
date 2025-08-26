/*
 * Copyright (C) 2003 Trevor van Bremen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1,
 * or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING.LIB.  If
 * not, write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307 USA
 */

#include	"isinternal.h"

int
isaudit (int ihandle, VB_CHAR *pcfilename, int imode)
{
    vb_rtd_t *vb_rtd =VB_GET_RTD;
	/* BUG - Write isaudit */
	if (ihandle < 0 || ihandle > vb_rtd->ivbmaxusedhandle) {
		vb_rtd->iserrno = EBADARG;
		return -1;
	}
	return 0;
    }
