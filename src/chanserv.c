/* src/chanserv.c - Diverses commandes sur le module chanserv
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
 * $Id: chanserv.c,v 1.98 2007/11/17 16:57:16 romexzf Exp $
 */

#include "main.h"
#include "hash.h"
#include "cs_cmds.h"
#include "outils.h"
#include "chanserv.h"
#include "config.h"
#include "add_info.h"
#include "del_info.h"

inline void show_accessn(anAccess *acces, anUser *user, aNick *num)
{
	csreply(num, GetReply(num, L_ACUSER), user->nick, acces->level);
	csreply(num, "\2Options:\2%s", GetAccessOptions(acces));
	if(AOnChan(acces)) csreply(num, GetReply(num, L_ACONCHAN));
	else csreply(num, GetReply(num, L_ACLASTSEEN), get_time(num, acces->lastseen));
	if(acces->info) csreply(num, "\2Infoline:\2 %s", acces->info);
}

int admin_say(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	putserv("%s " TOKEN_PRIVMSG " %s :%s",
		cs.num, parv[1], parv2msg(parc, parv, 2, 400));
	return 1;
}

int admin_do(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	putserv("%s " TOKEN_PRIVMSG " %s :\1ACTION %s\1",
		cs.num, parv[1], parv2msg(parc, parv, 2, 400));
	return 1;
}

