/* src/del_info.c - Suppression d'informations en mémoire
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
 * $Id: del_info.c,v 1.73 2007/12/16 20:48:15 romexzf Exp $
 */

#include "main.h"
#include "outils.h"
#include "del_info.h"
#include "hash.h"
#include "timers.h"
#include "debug.h"

void del_access(anUser *user, aChan *chan)
{
	anAccess **acces = &user->accesshead, *tmp;
	aLink *lp, **lpp;

	for(; (tmp = *acces); acces = &tmp->next)
		if(tmp->c == chan)
		{
			*acces = tmp->next;
			break;
		}

	for(lpp = &chan->access; (lp = *lpp); lpp = &lp->next)
		if(lp->value.a == tmp)
		{
			*lpp = lp->next;

			if(tmp->info) free(tmp->info);
			free(tmp);
			free(lp); /* ne pas oublier de free() le link, -memory leak-*/
			break;
		}
}

void del_link(aNChan *chan, aLink *link)
{
	if(link->next) link->next->last = link->last;
	if(link->last) link->last->next = link->next;
	else chan->members = link->next;
	free(link);

	if(0 == --chan->users)  /* channel gets empty */
	{	/* check if Z is still in here */
		if(chan->regchan && CJoined(chan->regchan)) chan->regchan->lastact = CurrentTS;
		else del_nchan(chan); /* otherwise destruct it */
	}
}

void del_join(aNick *nick, aNChan *chan)
{
	aJoin **joinp = &nick->joinhead, *join;

	for(; (join = *joinp); joinp = &join->next)
		if(chan == join->chan)
		{
			*joinp = join->next;

			if(nick->user && chan->regchan) /* update le lastseen comme il faut */
			{
				anAccess *acces = GetAccessIbyUserI(nick->user, chan->regchan);
				if(acces) acces->lastseen = CurrentTS;
			}
			del_link(chan, join->link); /* on del le link */
			free(join);
			break;
		}
}

void del_alljoin(aNick *nick)
{
	aJoin *join = nick->joinhead, *join_t = NULL;
	anAccess *a;

	for(; join; join = join_t)
	{
		join_t = join->next;
		if(join->chan->regchan && nick->user /* channel is registered, so is nick */
			&& (a = GetAccessIbyUserI(nick->user, join->chan->regchan))) /* got an access */
				a->lastseen = CurrentTS; /* update lastseen on channel */
		del_link(join->chan, join->link); /* clean link */
		free(join);
	}
	nick->joinhead = NULL;
}
