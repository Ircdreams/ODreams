/* include/del_info.h - Suppression d'informations en mémoire
 *
 * ODreams v2 (C) 2021 -- Ext by @bugsounet <bugsounet@bugsounet.fr>
 * site web: http://www.ircdreams.org
 *
 * Services pour serveur IRC. Supporté sur Ircdreams v3
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
 * $Id: del_info.h,v 1.9 2007/12/16 20:48:15 romexzf Exp $
 */

#ifndef HAVEINC_del_info
#define HAVEINC_del_info

extern void del_access(anUser *, aChan *);
extern void del_join(aNick *, aNChan *);
extern void del_link(aNChan *, aLink *);
extern void del_alljoin(aNick *);

#endif /*HAVEINC_del_info*/
