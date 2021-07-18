/* src/del_info.c - Suppression d'informations en mémoire
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
 * $Id: del_info.c,v 1.7 2005/05/17 01:29:19 bugs Exp $
 */

#include "main.h"
#include "outils.h"
#include "del_info.h"
#include "hash.h"

void del_bad(aBAD *bad)
{
        if(bad->next) bad->next->last = bad->last;
        if(bad->last) bad->last->next = bad->next;
        else badhead = bad->next;

        free(bad->mask);
        free(bad->raison);
        free(bad);
}

void del_ex(aEX *ex)
{
        if(ex->next) ex->next->last = ex->last;
        if(ex->last) ex->last->next = ex->next;
        else exhead = ex->next;

        free(ex->mask);
        free(ex->raison);
        free(ex);
}

void del_trusted(const char *ip)
{
        struct trusted *tmp, **tmpp = &trusthead;

        for(;(tmp = *tmpp);tmpp = &tmp->next)
                if(!strcasecmp(ip, tmp->ip))
                {
                        *tmpp = tmp->next;
                        free(tmp);
                        break;
                }
}

void del_gline(const char *host)
{
        struct gline *tmp, **tmpp = &glinehead;

        for(;(tmp = *tmpp);tmpp = &tmp->next)
                if(!strcasecmp(host, tmp->host))
                {
                        *tmpp = tmp->next;
                        free(tmp);
                        break;
                }
}

void del_trace(const char *mask)
{
        struct trace *tmp, **tmpp = &tracehead;

        for(;(tmp = *tmpp);tmpp = &tmp->next)
                if(!strcasecmp(mask, tmp->mask))
                {
                        *tmpp = tmp->next;
                        free(tmp);
                        break;
                }
}

void del_shun(const char *host)
{
        struct shun *tmp, **tmpp = &shunhead;

        for(;(tmp = *tmpp);tmpp = &tmp->next)
                if(!strcasecmp(host, tmp->host))
                {
                        *tmpp = tmp->next;
                        free(tmp);
                        break;
                }
}

void del_link(aChan *chan, aNick *nick) 
{ 
        register aLink **lpp = &chan->members, *lp = NULL; 
    
        --chan->users; 
        for(;(lp = *lpp);lpp = &lp->next) 
                if(lp->value.j->nick == nick) 
                { 
                        *lpp = lp->next; 
                        free(lp); 
                        break; 
                } 
} 

void del_join(aNick *nick, const char *chan)
{
	aJoin **joinp = &nick->joinhead, *join;

	for(;(join = *joinp);joinp = &join->next)
		if(!strcasecmp(chan, join->channel))
		{
			*joinp = join->next;

			if(join->chan) del_link(join->chan, nick); /* on del le link*/
			free(join);
			break;
		}
}
