/* include/del_info.h - Suppression d'informations en mémoire
 * Copyright (C) 2004 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://ircdreams.org
 *
 * Services pour serveur IRC. Supporté sur IrcDreams V.2
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
 * $Id: del_info.h,v 1.7 2005/05/17 01:29:19 bugs Exp $
 */

#ifndef HAVEINC_del_info
#define HAVEINC_del_info

extern void del_join(aNick *, const char *);
extern void del_link(aChan *, aNick *);
extern void del_trusted(const char *);
extern void del_gline(const char *);
extern void del_shun(const char *);

extern void del_bad(aBAD *);
extern void del_ex(aEX *);

extern void del_trace(const char *);

#endif /*HAVEINC_del_info*/
