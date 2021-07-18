/* src/hash_chan.c - gestion des hash chan
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
 * $Id: hash_chan.c,v 1.4 2005/05/07 22:41:13 bugs Exp $
 */

#include "main.h"
#include "hash.h"
#include "outils.h"
#include "debug.h"
#include "del_info.h"
#include "os_cmds.h"
#include "fichiers.h"

aChan *getchaninfo(const char *chan)
{
	unsigned int hash = do_hashc(chan);
	register aChan *tmp = chan_tab[hash];
	for(;tmp && strcasecmp(chan, tmp->chan);tmp = tmp->next);
	return tmp;
}
