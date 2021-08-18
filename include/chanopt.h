/* include/chanopt.h
 * Copyright (C) 2002-2003 Inter System
 *
 * ODreams v2 (C) 2021 -- Ext by @bugsounet <bugsounet@bugsounet.fr>
 * site web: http://www.ircdreams.org
 *
 * Services pour serveur IRC. Support� sur Ircdreams v3
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
 * $Id: chanopt.h,v 1.1 2003/12/12 17:44:58 romexzf Exp $
 */

#ifndef HAVEINC_chanopt
#define HAVEINC_chanopt

extern int generic_chanopt(aNick *, aChan *, int, char **);
extern int locktopic(aNick *, aChan *, int, char **);
extern int strictop(aNick *, aChan *, int, char **);
extern int nobans(aNick *, aChan *, int, char **);
extern int noops(aNick *, aChan *, int, char **);
extern int deftopic(aNick *, aChan *, int, char **);
extern int defmodes(aNick *, aChan *, int, char **);
extern int description(aNick *, aChan *, int, char **);
extern int banlevel(aNick *, aChan *, int, char **);
extern int bantype(aNick *, aChan *, int, char **);
extern int define_motd(aNick *, aChan *, int, char **);
extern int chanurl(aNick *, aChan *, int, char **);

#endif /*HAVEINC_chanopt*/
