/* include/hash.h
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
 * $Id: hash.h,v 1.7 2005/05/22 15:58:51 bugs Exp $
 */

#ifndef HAVEINC_hash
#define HAVEINC_hash

/* 	doivent être des multiples de 2 (1024, 512, 256)
	pour pouvoir (&)and-er au lieu de (%)moduler*/
#define CHANHASHSIZE 512  /* taille de la hash des chan avec une hash de 200 et 200 chan regs
						131 offsets utilisée, temps réduit par 10 pour getchaninfo()*/
#define USERHASHSIZE 2048
#define NICKHASHSIZE 1024
#define CMDHASHSIZE 140

/* chan */
unsigned int do_hashc(const char *);
extern aChan *getchaninfo(const char *);

/* nick */
extern unsigned int base64toint(const char *);
extern aNick *num2nickinfo(const char *);
extern aNick *getnickbynick(const char *);
extern aNick *add_nickinfo(const char *, const char *, const char *, const char *, const char *, aServer *, const char *, time_t, const char *);
extern void del_nickinfo(const char *, const char *);
extern int switch_nick(aNick *, const char*);
extern int purge_nickandserv(void);

/* user */
extern anUser *getuserinfo(const char *);
extern anUser *add_regnick(const char *, int);
extern void del_regnick(anUser *);

/* cmd */
extern int RegisterCmd(const char *, int, int, int, int (*) (aNick *, aChan *, int, char **));
extern int HashCmd_switch(aHashCmd *, const char *);
extern aHashCmd *FindCommand(const char *);
extern aHashCmd *FindCoreCommand(const char *);

/* misc */
extern anUser *GetUserIbyMail(const char *); 
extern int ChanLevelbyUserI(anUser *, aChan *); 
extern aJoin *getjoininfo(aNick *, const char *); 
extern aJoin *GetJoinIbyC(aNick *, aChan *); 
extern char *IsAnOwner(anUser *); 
extern anAccess *GetAccessIbyUserI(anUser *, aChan *); 
extern aNick *GetMemberIbyNum(aChan *, const char *); 
extern aNick *GetMemberIbyNick(aChan *, const char *); 
extern aServer *num2servinfo(const char *); 
extern aServer *GetLinkIbyServ(const char *); 

aBAD *find_bad(const char *, int);
aEX *find_ex(const char *, int);

#endif /*HAVEINC_hash*/
