/* src/cs_register.c - Enregistrement de salon
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
 * $Id: cs_register.c,v 1.66 2007/01/02 19:44:30 romexzf Exp $
 */

#include <ctype.h>
#include "main.h"
#include "hash.h"
#include "add_info.h"
#include "fichiers.h"
#include "outils.h"
#include "cs_cmds.h"
#include "config.h"
#include "crypt.h"
#include "data.h"

/* regchan, unreg (+chancheck) + register, drop */

int chan_check(const char *chan, aNick *nick)
{
	int i = 1;

	if(*chan != '#') return csreply(nick, GetReply(nick, L_CHANNELNAME));

	for(; chan[i] && i < REGCHANLEN; ++i)
		if(chan[i] == ',' || !isascii(chan[i])) /* bad char or comma */
			return csreply(nick, GetReply(nick, L_USER_INVALID), chan);
	/* if chan[i] != \0 then i > REGCHANLEN */
	if(chan[i])	return csreply(nick, GetReply(nick, L_CHANMAXLEN), REGCHANLEN);

	return 1;
}

int first_register(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  anUser *user = NULL;
  aNick *n;
  aHashCmd *cmdp = FindCommand("REGISTER");

  if (!(n = getnickbynick(parv[1])))
    return csntc(nick, "No such nick: %s", parv[1]);

  if(!(n->flag & N_OPER))
    return csntc(nick, "Permission Denied: You are not an IRC Operator !");

  if(!GetConf(CF_PREMIERE))
    return csntc(nick, "Reserved for the first launch!");

  if((user = getuserinfo(parv[1])))
    return csntc(nick, "%s is already registered", user->nick);

  user = add_regnick(parv[1], 10);

  csntc(nick, "Welcome to ODreams IRC Service [" SPVERSION "] !");
  csntc(nick, "You are a maximum level administrator.");

  ConfFlag &= ~CF_PREMIERE;
  cmdp->flag |= CMD_DISABLE;

  db_write_users();

  user = getuserinfo(parv[1]);
  nick->user = user;
  nick->user->n = nick;

  return 1;
}
