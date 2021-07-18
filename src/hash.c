/* src/hash.c dohash()
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
 * $Id: hash.c,v 1.4 2005/05/15 16:31:00 bugs Exp $
 */

#include "main.h"
#include "hash.h"
#include "outils.h"
#include <ctype.h>

unsigned int do_hashc(const char *chan)
{
	unsigned int checksum = 0;
	while(*chan) checksum += (checksum << 3) + tolower(*chan++);
	return checksum & (CHANHASHSIZE-1);
}

int ChanLevelbyUserI(anUser *user, aChan *chan)
{
	anAccess *acces = GetAccessIbyUserI(user, chan);
	return acces ? acces->level : 0;
}

aJoin *getjoininfo(aNick *nick, const char *chan)
{
	register aJoin *join = nick->joinhead;
	for(;join && strcasecmp(chan, join->channel);join = join->next);
	return join;
}

aJoin *GetJoinIbyC(aNick *nick, aChan *chan)
{
	register aJoin *join = nick->joinhead;
	for(;join && chan != join->chan;join = join->next);
	return join;
}

aBAD *find_bad(const char *pattern, int type)
{
        aBAD *bad = badhead;

        for(;bad;bad = bad->next)
                if((type & bad->flag) & (BAD_TYPECHAN|BAD_TYPEUSER)
                        && ((type & BAD_MASK && !mmatch(bad->mask, pattern))
                        || (!(type & BAD_MASK) && !strcasecmp(bad->mask, pattern)))) break;
        return bad;
}

aEX *find_ex(const char *pattern, int type)
{
        aEX *ex = exhead;

        for(;ex;ex = ex->next)
                if((type & ex->flag) & (EX_TYPECHAN|EX_TYPEUSER)
                        && ((type & EX_MASK && !mmatch(ex->mask, pattern))
                        || (!(type & EX_MASK) && !strcasecmp(ex->mask, pattern)))) break;
        return ex;
}

anAccess *GetAccessIbyUserI(anUser *user, aChan *chan)
{
	register anAccess *a = user->accesshead;
	for(;a && a->c != chan; a = a->next);
	return a ? a : NULL;
}

aNick *GetMemberIbyNick(aChan *chan, const char *nick)
{
	aLink *l = chan->members;
	for(;l && strcasecmp(l->value.j->nick->nick, nick);l = l->next);
	return l ? l->value.j->nick : NULL;
}

aServer *num2servinfo(const char *num)
{
	unsigned int servindex = base64toint(num);

	if(servindex > MAXNUM-1) return NULL;
	else return serv_tab[servindex];
}

aServer *GetLinkIbyServ(const char *serv)
{
	int i = 0;
	for(;i < MAXNUM;++i) if(serv_tab[i] && !strcasecmp(serv_tab[i]->serv, serv)) return serv_tab[i];
	return NULL;
}
