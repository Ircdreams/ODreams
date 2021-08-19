/* src/hash_user.c - gestion des hash user
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
 * $Id: hash_user.c,v 1.27 2008/01/05 18:34:13 romexzf Exp $
 */

#include "main.h"
#include "hash.h"
#include "debug.h"
#include "outils.h"
#include "mylog.h"
#include "fichiers.h"
#include "del_info.h"
#include "cs_cmds.h"
#include <ctype.h>
#include "timers.h"
#include "data.h"

static anUser *mail_tab[USERHASHSIZE];
static anUser *userid_tab[USERHASHSIZE];

unsigned long user_maxid = 0UL;

static inline unsigned int do_hashu(const char *user)
{
	unsigned int checksum = 0;
	while(*user) checksum += (checksum << 3) + tolower((unsigned char) *user++);
	return checksum & (USERHASHSIZE-1);
}

static int hash_deluser(anUser *user)
{
	unsigned int hash = do_hashu(user->nick);
	anUser *tmp = user_tab[hash];

	if(tmp == user) user_tab[hash] = user->next;
	else
	{
		for(; tmp && tmp->next != user; tmp = tmp->next);
		if(tmp) tmp->next = user->next;
		else Debug(W_WARN|W_MAX, "H_del_user %s non trouvé à l'offset %u ?!", user->nick, hash);
	}
	return 0;
}

static int hash_adduser(anUser *user)
{
	unsigned int hash = do_hashu(user->nick);
	user->next = user_tab[hash];
	user_tab[hash] = user;
	return 0;
}

int switch_user(anUser *user, const char *newuser)
{
	hash_deluser(user);
	Strncpy(user->nick, newuser, NICKLEN);
	hash_adduser(user);
	if(user->n) cs_account(user->n, user);
	return 0;
}

anUser *getuserinfo(const char *nick)
{
	unsigned int hash = do_hashu(nick);
	register anUser *tmp = user_tab[hash];
	for(; tmp && strcasecmp(nick, tmp->nick); tmp = tmp->next);
	return tmp;
}

static int hash_delmail(anUser *user)
{
	unsigned int hash = do_hashu(user->mail);
	anUser *tmp = mail_tab[hash];

	if(tmp == user) mail_tab[hash] = user->mailnext;
	else
	{
		for(; tmp && tmp->mailnext != user; tmp = tmp->mailnext);
		if(tmp) tmp->mailnext = user->mailnext;
		else Debug(W_WARN|W_MAX, "H_del_mail %s non trouvé à l'offset %u ?!", user->mail, hash);
	}
	return 0;
}

static int hash_addmail(anUser *user)
{
	unsigned int hash = do_hashu(user->mail);
	user->mailnext = mail_tab[hash];
	mail_tab[hash] = user;
	return 0;
}

int switch_mail(anUser *user, const char *newmail)
{
	hash_delmail(user);
	Strncpy(user->mail, newmail, MAILLEN);
	hash_addmail(user);
	return 0;
}

anUser *GetUserIbyMail(const char *mail)
{
	unsigned int hash = do_hashu(mail);
	register anUser *tmp = mail_tab[hash];
	for(; tmp && strcasecmp(mail, tmp->mail); tmp = tmp->mailnext);
	return tmp;
}

static int hash_addid(anUser *user)
{
	unsigned int hash = user->userid & (USERHASHSIZE-1);
	user->idnext = userid_tab[hash];
	userid_tab[hash] = user;
	return 0;
}

static int hash_delid(anUser *user)
{
	unsigned int hash = user->userid & (USERHASHSIZE-1);
	anUser *tmp = userid_tab[hash];

	if(tmp == user) userid_tab[hash] = user->idnext;
	else
	{
		for(; tmp && tmp->idnext != user; tmp = tmp->idnext);
		if(tmp) tmp->idnext = user->idnext;
		else Debug(W_WARN|W_MAX, "H_del_userid %s non trouvé à l'offset %u ?!", user->nick, hash);
	}
	return 0;
}

anUser *GetUserIbyID(unsigned long userid)
{
	register anUser *tmp = userid_tab[(userid & (USERHASHSIZE-1))];
	for(; tmp && tmp->userid != userid; tmp = tmp->idnext);
	return tmp;
}

anUser *add_regnick(const char *user, int level)
{
	anUser *u = calloc(1, sizeof *u);

	if(!u)
	{
		Debug(W_MAX|W_WARN, "add_regnick, malloc a échoué pour anUser %s", user);
		return NULL;
	}

	Strncpy(u->nick, user, NICKLEN);
	u->level = level;

	hash_adduser(u);
	return u;
}

anUser *add_regnick(const char *user, int level)
{
  anUser *new;
  unsigned int hash = do_hashu(user);

  if(!(new = calloc(1, sizeof *new)))
  {
    Debug(W_MAX|W_WARN, "add_regnick, malloc a échoué pour anUser %s", user);
    return NULL;
  }

  Strncpy(new->nick, user, NICKLEN);
  new->level = level;

  new->next = user_tab[hash];
  user_tab[hash] = new;
  return new;
}

void del_regnick(anUser *user, int flag, const char *raison)
{
	anAccess *acces, *acces_t;

	for(acces = user->accesshead; acces; acces = acces_t)
	{
		acces_t = acces->next;
		if(AOwner(acces)) del_chan(acces->c, flag|HF_LOG, raison);
		else del_access(user, acces->c);
	}

 	if(flag & HF_LOG)
		log_write(LOG_UCMD, 0, "purge %s (%d)", user->nick, CurrentTS - user->lastseen);

	hash_deluser(user);
	hash_delmail(user);
	hash_delid(user);
	if(user->suspend) data_free(user->suspend);
	if(user->cantregchan) data_free(user->cantregchan);
	if(user->nopurge) data_free(user->nopurge);
	if(user->lastlogin) free(user->lastlogin);
	if(user->cookie) free(user->cookie);
	free(user);
}
