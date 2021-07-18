/* src/hash_user.c - gestion des hash user
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
 * $Id: hash_user.c,v 1.5 2005/06/16 17:21:57 bugs Exp $
 */

#include "main.h"
#include "hash.h"
#include "debug.h"
#include "outils.h"
#include "fichiers.h"
#include "del_info.h"
#include "os_cmds.h"
#include <ctype.h>

static inline unsigned int do_hashu(const char *user)
{
	unsigned int checksum = 0;
	while(*user) checksum += (checksum << 3) + tolower(*user++);
	return checksum & (USERHASHSIZE-1);
}

static int hash_deluser(anUser *user)
{
	unsigned int hash = do_hashu(user->nick);
	anUser *tmp = user_tab[hash];

	if(tmp == user) user_tab[hash] = user->next;
	else
	{
		for(;tmp && tmp->next != user;tmp = tmp->next);
		if(tmp) tmp->next = user->next;
		else Debug(W_WARN|W_MAX, "H_del_user %s non trouvé à l'offset %u ?!", user->nick, hash);
	}
	return 0;
}

anUser *getuserinfo(const char *nick)
{
	unsigned int hash = do_hashu(nick);
	register anUser *tmp = user_tab[hash];
	for(;tmp && strcasecmp(nick, tmp->nick);tmp=tmp->next);
	return tmp;
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

void del_regnick(anUser *user)
{
        hash_deluser(user);
        free(user);
	db_write_users();
}
