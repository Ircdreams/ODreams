/* src/admin_chan.c - commandes admin pour gerer les salons
 *
 * Copyright (C) 2002-2008 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@coderz.info>
 *                         Benjamin Beret <kouak@kouak.org>
 *
 * SDreams v2 (C) 2021 -- Ext by @bugsounet <bugsounet@bugsounet.fr>
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
 * $Id: admin_chan.c,v 1.100 2008/01/05 01:24:13 romexzf Exp $
 */

#include "main.h"
#include "outils.h"
#include "mylog.h"
#include "cs_cmds.h"
#include "fichiers.h"
#include "admin_chan.h"
#include "add_info.h"
#include "del_info.h"
#include "hash.h"
#include "cs_register.h"
#include "data.h"

#define MAXMATCHES 40

int whoison(aNick *nick, aChan *chan, int parc, char **parv)
{
	unsigned int first = 1, used = 0;
	char buf[450] = {0};
	aLink *lp = chan->netchan->members;

#define ITEM_SIZE (NICKLEN + 16) /* nick + prefix *!@+ */

	if(!lp) return csreply(nick, "Aucun User sur %s.", chan->chan);

	for(; lp; lp = lp->next)
	{
		aJoin *j = lp->value.j;

		if(used + ITEM_SIZE >= sizeof buf)
		{
			if(first) csreply(nick, "Un total de %d Users sur %s:%s...",
							chan->netchan->users, chan->chan, buf);
			else csreply(nick, "              :%s...", buf);
			first = used = 0;
		}
		used += fastfmt(buf + used, " $$", GetChanPrefix(j->nick, j), j->nick->nick);
	}

	if(first && *buf) csreply(nick, "Un total de %d Users sur %s:\2%s",
							chan->netchan->users, chan->chan, buf);
	else if(*buf) csreply(nick, "              :%s", buf);

	return 1;
}
