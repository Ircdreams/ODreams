/* include/main.h - Déclarations principales
 * Copyright (C) 2004-2005 ircdreams.org
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
 * $Id: main.h,v 1.31 2005/12/16 10:35:42 bugs Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/select.h>
#include "../config.h"
#include "token.h"

/* Version de ODreams */

#define PATCH_LEVEL "0+DEV"
#define RELEASE_CANDIDATE "0"
#define BASE_SERVICE "2"

#define SD_VERSION BASE_SERVICE "." RELEASE_CANDIDATE "." PATCH_LEVEL

#define SPVERSION SD_VERSION

/* Variables */

#define CTCP_DELIM_CHAR '\001'
#define NICKLEN 45
#define TOPICLEN 250
#define HOSTLEN 63
#define REALEN 50
#define USERLEN 10
#define KEYLEN 23
#define CHANLEN 200
#define CMDLEN 14
#define SYNTAXLEN 100
#define NUHLEN (NICKLEN + USERLEN + HOSTLEN + 3)
#define REGCHANLEN 100
#define TIMELEN 40

#define MAXADMLVL 	10 /* level max des admins */
#define ADMINLEVEL 	1 /* level min des admins, level entre USER&ADMIN LEVEL ~= Helpers? */

#define CMDS_FILE DBDIR"/cmds.db"
#define DBUSERS "users.db"
#define TRUSTED_FILE DBDIR"/trusted.db"
#define GLINE_FILE DBDIR"/gline.db"
#define SHUN_FILE DBDIR"/shun.db"
#define TRACE_FILE DBDIR"/trace.db"
#define ALARMINTERVAL 25 /* interval de l'alarme (si Z rame beaucoup.. l'augmenter)*/
#define ODREAMS_PID "odreams.pid" /* fichier où sera stocké le n° de pid de odreams */

#define WAIT_CONNECT 10 /* Temps qu'attend le serveur avant de retenter une connexion */

#define IsAdmin(x)              ((x)->level >= ADMINLEVEL)
#define IsAnAdmin(x)    	    ((x) && IsAdmin(x)) /* x est un admin ? */
#define IsOper(x) 		        ((x)->flag & N_OPER)
#define IsOperOrService(x)      ((x)->flag & (N_OPER|N_SERVICE))
#define IsService(x)		    ((x)->flag & N_SERVICE)
#define IsAway(x) 		        ((x)->flag & N_AWAY)
#define IsHelper(x)		        ((x)->flag & N_HELPER)
#define IsHiding(x)		        ((x)->flag & N_HIDE)

#define MAXNUM 4096
#define NUMSERV 2

#include <structs.h>

#define IsOp(x) 		((x)->status & J_OP)
#define IsHalf(x)		((x)->status & J_HALF)
#define IsVoice(x) 		((x)->status & J_VOICE)
#define DoOp(x) 		((x)->status |= J_OP)
#define DoHalf(x)		((x)->status |= J_HALF)
#define DoVoice(x) 		((x)->status |= J_VOICE)
#define DeOp(x) 		((x)->status &= ~J_OP)
#define DeHalf(x)		((x)->status &= ~J_HALF)
#define DeVoice(x) 		((x)->status &= ~J_VOICE)

#define HasMode(chan, flag) 	((chan)->mode.modes & (flag))
#define HasDMode(chan, flag)    ((chan)->defmodes.modes & (flag))
#define ChanCmd(x) 				((x)->flag & CMD_CHAN)
#define AdmCmd(x) 				((x)->flag & CMD_ADMIN)
#define NeedNoAuthCmd(x)        ((x)->flag & CMD_NEEDNOAUTH)
#define SecureCmd(x)            ((x)->flag & CMD_SECURE)
#define Secure2Cmd(x)           ((x)->flag & CMD_SECURE2)
#define HelpCmd(x)			    ((x)->flag & CMD_HELPER)

#define GetConf(x) 				(ConfFlag & (x))
#define ASIZE(x) 				(sizeof (x) / sizeof *(x))

#define SPECIAL_CHAR "{}[]|-_\\^`.()<>"

/* global vars */
extern struct robot bot;
extern int ConfFlag;
extern struct bots os;
extern aHashCmd *cmd_hash[];
extern anUser *user_tab[];
extern aChan *chan_tab[];
extern aNick **num_tab[];
extern aServer *serv_tab[];
extern aNick *nick_tab[];

extern int running;
extern int deconnexion;
extern int complete;
extern time_t CurrentTS;

extern struct nickinfo *nickhead;
extern struct trusted *trusthead;
extern struct gline *glinehead;
extern struct shun *shunhead;
extern struct trace *tracehead;

extern int CmdsCount; 

#define BAD_FILE DBDIR"/bad.db"
#define IsBadNick(nick)          find_bad(nick, BAD_TYPEUSER|BAD_MASK)
#define IsBadChan(chan)          find_bad(chan, BAD_TYPECHAN|BAD_MASK)
extern aBAD *badhead;

#define EX_FILE DBDIR"/ex.db"
#define IsExNick(nick)          find_ex(nick, EX_TYPEUSER|EX_MASK)
#define IsExChan(chan)          find_ex(chan, EX_TYPECHAN|EX_MASK)
extern aEX *exhead;

#define HasWildCard(string)      (strchr((string), '*') || strchr((string), '?'))
