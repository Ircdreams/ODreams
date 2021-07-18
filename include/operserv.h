/* include/operserv.h
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
 * $Id: operserv.h,v 1.11 2005/05/25 17:56:56 bugs Exp $
 */

extern int first_register(aNick *, aChan *, int, char **);
extern int user(aNick *, aChan *, int, char **);
extern int xop(aNick *, aChan *, int, char **);
extern int xdeop(aNick *, aChan *, int, char **);
extern int xvoice(aNick *, aChan *, int, char **);
extern int xdevoice(aNick *, aChan *, int, char **);
extern int xhalf(aNick *, aChan *, int, char **);
extern int xdehalf(aNick *, aChan *, int, char **);
extern int xkick(aNick *, aChan *, int, char **);
extern int xkill(aNick *, aChan *, int, char **);
extern int xblock(aNick *, aChan *, int, char **);
extern int silence(aNick *, aChan *, int, char **);
extern int xgline(aNick *, aChan *, int, char **);
extern int xshun(aNick *, aChan *, int, char **);
extern int xsay(aNick *, aChan *, int, char **);
extern int xdo(aNick *, aChan *, int, char **);
extern int mjoin(aNick *, aChan *, int, char **);
extern int mpart(aNick *, aChan *, int, char **);
extern int mgline(aNick *, aChan *, int, char **);
extern int mshun(aNick *, aChan *, int, char **);
extern int mkill(aNick *, aChan *, int, char **);
extern int mkick(aNick *, aChan *, int, char **);
extern int mop(aNick *, aChan *, int, char **);
extern int mdeop(aNick *, aChan *, int, char **);
extern int mvoice(aNick *, aChan *, int, char **);
extern int mdevoice(aNick *, aChan *, int, char **);
extern int mhalf(aNick *, aChan *, int, char **);
extern int mdehalf(aNick *, aChan *, int, char **);
extern int killhost(aNick *, aChan *, int, char **);
extern int closechan(aNick *, aChan *, int, char **);
extern int xban(aNick *, aChan *, int, char **);
extern int xunban(aNick *, aChan *, int, char **);
extern int xmode(aNick *, aChan *, int, char **);
extern int check(aNick *, aChan *, int, char **);
extern int trust(aNick *, aChan *, int, char **);
extern int stats(aNick *, aChan *, int, char **); 
extern int trace(aNick *, aChan *, int, char **);
extern int ircop(aNick *, aChan *, int, char **);
extern int helper(aNick *, aChan *, int, char **);
extern int svsjoin(aNick *, aChan *, int, char **);
extern int svspart(aNick *, aChan *, int, char **);
extern int svsnick(aNick *, aChan *, int, char **);
extern int svshost(aNick *, aChan *, int, char **);
extern int wall(aNick *, aChan *, int, char **);
