/* include/admin_manage.h
 * Copyright (C) 2004 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://ircdreams.org
 *
 * Services pour serveur IRC. Support? sur IrcDreams V.2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * $Id: admin_manage.h,v 1.2 2005/05/22 15:58:51 bugs Exp $
 */

#ifndef HAVEINC_admin_manage
#define HAVEINC_admin_manage

extern aNick **adminlist; 
extern int adminmax; 
    
extern int adm_active_add(aNick *); 
extern int adm_active_del(aNick *); 

#endif /*HAVEINC_admin_manage*/
