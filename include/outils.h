/* include/outils.h - Divers outils
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
 * $Id: outils.h,v 1.4 2005/05/15 23:31:52 bugs Exp $
 */

#ifndef HAVEINC_outils
#define HAVEINC_outils

#define LISTSEP ','

extern int match(const char *, const char *); /* hack*/
extern int mmatch(const char *, const char *); /* hack*/

extern int is_num(const char *);
extern int is_ip(const char *);
extern void strupr(char *);
extern void strlwr(char *);
extern void strip_newline(char *);

extern struct trusted *gettrusted(const char *);
extern struct gline *getgline(const char *);
extern struct shun *getshun(const char *);

extern int getoption(const char *, char **, int, int, int);

extern int split_buf(char *, char **, int);
extern char *parv2msg(int, char **, int, int);

extern char *duration(int);
extern time_t convert_duration(const char *);
extern char *get_time(aNick *, time_t);

extern char *GetNUHbyNick(aNick *, int);
extern char *GetPrefix(aNick *);
extern char *GetIP(const char *);

extern void putlog(const char *, const char *, ...);

extern int parse_umode(int, const char *);
extern char *GetModes(int);
extern int cmodetoflag(int, const char *);

extern int IsValidNick(char *);

extern char *RealCmd(const char *);

extern char *str_dup(char **, const char *);
extern char *Strncpy(char *, const char *, size_t);
extern char *Strtok(char **, char *, int);

extern int fastfmt(char *, const char *, ...); 
extern int fastfmtv(char *, const char *, va_list); 

extern char *pretty_mask(char *);

#endif /*HAVEINC_outils*/
