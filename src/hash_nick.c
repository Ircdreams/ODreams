/* src/hash_nick.c - gestion des hash nick
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
 * $Id: hash_nick.c,v 1.4 2005/06/16 17:21:57 bugs Exp $
 */

#include <ctype.h>
#include "main.h"
#include "outils.h"
#include "admin_manage.h"
#include "hash.h"
#include "del_info.h"
#include "debug.h"
#include "os_cmds.h"
#include "fichiers.h"
 
static const unsigned char convert2n[] = {
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,52,53,54,55,
   56,57,58,59,60,61, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,
   13,14,15,16,17,18,19,20,21,22,23,24,25,62, 0,63, 0, 0, 0,26,27,28,29,30,31,32,
   33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51, 0, 0, 0, 0, 0

};

static inline unsigned int do_hashn(const char *nick) 
{ 
        unsigned int checksum = 0; 
        while(*nick) checksum += (checksum << 3) + tolower(*nick++); 
        return checksum & (NICKHASHSIZE-1); 
} 

unsigned int base64toint(const char *s)
{
    unsigned int i = 0;
    while (*s) i = (i << 6) + convert2n[(unsigned char) *s++];
    return i;
}

static int hash_delnick(aNick *nick) 
{ 
        unsigned int hash = do_hashn(nick->nick); 
        aNick *tmp = nick_tab[hash]; 
 
        if(tmp == nick) nick_tab[hash] = nick->next; 
        else 
        { 
                for(;tmp && tmp->next != nick;tmp = tmp->next); 
                if(tmp) tmp->next = nick->next; 
                else Debug(W_WARN|W_MAX, "H_del_nick %s non trouvé à l'offset %u ?!", nick->nick, hash); 
        } 
        return 0; 
} 
    
int switch_nick(aNick *nick, const char *newnick) 
{ 
        unsigned int hash = do_hashn(newnick); 
    
        hash_delnick(nick); 
        Strncpy(nick->nick, newnick, NICKLEN); 
        nick->next = nick_tab[hash]; 
        nick_tab[hash] = nick; 
        return 0; 
} 


aNick *num2nickinfo(const char *num)
{
	unsigned int servindex = convert2n[(unsigned char)num[1]] + 64 * convert2n[(unsigned char)num[0]],
	numindex = (convert2n[(unsigned char)num[4]] + 64 * convert2n[(unsigned char)num[3]] + 4096 * convert2n[(unsigned char)num[2]]) & serv_tab[servindex]->smask;
	return num_tab[servindex][numindex];
}

aNick *getnickbynick(const char *nick)
{
	unsigned int hash = do_hashn(nick);
	register aNick *tmp = nick_tab[hash];
	for(;tmp && strcasecmp(nick, tmp->nick);tmp = tmp->next);
	return tmp;
}

aNick *add_nickinfo(const char *nick, const char *user, const char *host, const char *base64,
			const char *num, aServer *server, const char *name, time_t ttmco, const char *umode)
{
	aNick *n = calloc(1, sizeof *n);
	unsigned int servindex = convert2n[(unsigned char)num[1]] + 64 * convert2n[(unsigned char)num[0]],
	numindex = (convert2n[(unsigned char)num[4]] + 64 * convert2n[(unsigned char)num[3]] + 4096 * convert2n[(unsigned char)num[2]]) & server->smask,
	hash = do_hashn(nick);

	if(!n)
	{
		Debug(W_MAX|W_WARN, "add_nickinfo, malloc a échoué pour aNick %s", nick);
		return NULL;
	}

	if(num_tab[servindex][numindex])
	{
		aNick *nptr = num_tab[servindex][numindex];
		Debug(W_DESYNCH|W_WARN, "add_nickinfo, Offset pour %s(%s@%s) [%s] déjà pris par %s(%s@%s) [%s]",
			nick, user, host, num, nptr->nick, nptr->ident, nptr->host, nptr->numeric);
		free(n);
		return NULL;
	}

	num_tab[servindex][numindex] = n;/* table des pointeurs num -> struct*/

	Strncpy(n->nick, nick, NICKLEN);
	Strncpy(n->ident, user, USERLEN);
	Strncpy(n->host, host, HOSTLEN);
	Strncpy(n->base64, base64, 7);
	strcpy(n->numeric, num);
	n->serveur = server;
	Strncpy(n->name, name, REALEN);
	n->ttmco = ttmco;
	n->floodtime = CurrentTS;
	n->floodcount = 0;

	if(umode) n->flag = parse_umode(0, umode);
	
	n->next = nick_tab[hash];/* hash table des nick -> struct*/
	nick_tab[hash] = n;
	return n;
}

void del_nickinfo(const char *num, const char *event)
{
	aJoin *join, *join_t = NULL;
	aNick *nick;
	unsigned int servindex = convert2n[(unsigned char)num[1]] + 64 * convert2n[(unsigned char)num[0]],
	numindex = convert2n[(unsigned char)num[4]] + 64 * convert2n[(unsigned char)num[3]] + 4096 * convert2n[(unsigned char)num[2]];

	if(!serv_tab[servindex])
	{
		Debug(W_DESYNCH|W_WARN, "del_nickinfo(%s), del %s sur un serveur inconnu!", event, num);
		return;
	}
	numindex &= serv_tab[servindex]->smask;/* on réduit par le max du serveur (attribution des nums..)*/

	if(!num_tab[servindex][numindex])
	{
		Debug(W_DESYNCH|W_WARN, "del_nickinfo(%s), del %s membre inconnu", event, num);
		return;
	}

	nick = num_tab[servindex][numindex];

	for(join = nick->joinhead;join;join = join_t)
	{
		join_t = join->next;
		if(join->chan) /* c'est un chan reg (économie d'un getchaninfo..)*/
		{
			del_link(join->chan, nick);
		}
		free(join);
	}

	if(nick->user)	/* sauvergarde du lastlogin */
	{
		db_write_users();
		if(IsAdmin(nick->user)) adm_active_del(nick);
		nick->user->n = NULL;
	}

	num_tab[servindex][numindex] = NULL;/* tab des num purgée*/
	hash_delnick(nick);/* swap du nick dans la hash table*/
	free(nick);
}

int purge_nickandserv(void)
{
	int i = 0, j;
	for(;i < MAXNUM;i++)
		if(num_tab[i])
		{
			for(j = 0;j < serv_tab[i]->maxusers;j++)
				if(num_tab[i][j]) del_nickinfo(num_tab[i][j]->numeric, "clearing");

			free(serv_tab[i]);
			free(num_tab[i]);
			num_tab[i] = NULL;
			serv_tab[i] = NULL;
		}

	return 0;
}
