/* src/admin_user.c - commandes admins pour gerer les users
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
 * $Id: admin_user.c,v 1.4 2005/06/16 17:21:57 bugs Exp $
 */

#include <sys/time.h>
#include "main.h"
#include "outils.h"
#include "hash.h"
#include "os_cmds.h"
#include "fichiers.h"
#include "divers.h"
#include "admin_user.h"
#include "config.h"

int os_whois(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	char chans[451] = {0}, b[CHANLEN + 4] = {0};
	int size = 0, count = 0;
	anUser *uptr;
	aJoin *join;
	aNick *nptr = getnickbynick(parv[1]);

	if(!nptr) return osntc(nick, "%s n'est pas connecté.", parv[1]);

	osntc(nick, "Infos sur \2%s", nptr->nick);
	osntc(nick, " Nom: %s", nptr->name);
	osntc(nick, " Mask: %s\2%s\2!\002%s\2@\002%s", GetPrefix(nptr), nptr->nick, nptr->ident, nptr->host);

	osntc(nick, " IP:\2 %s", GetIP(nptr->base64));
	if(IsOper(nptr)) osntc(nick, " %s est \2IRCop.", nptr->nick);
	osntc(nick, " UserModes: \2%s\2", (nptr->flag & N_UMODES) ? GetModes(nptr->flag) : "Aucun");

	for(join = nptr->joinhead;join; join = join->next)
	{
		int tmps = size, p = fastfmt(b, "$$$$ ", IsVoice(join) ? "+" : "", IsHalf(join) ? "%" : "", IsOp(join) ? "@" : "", join->channel);
		size += p;
		if(size >= 450) /* le dernier fait depasser le buffer */
		{
			osntc(nick, count++ ? "%s ...": " Présent sur %s ...", chans);
			strcpy(chans, b); /* ajoute le suivant pour la prochaine*/
			size = p;
		}
		else strcpy(&chans[tmps], b);
	}

	osntc(nick, !count ? " Présent sur %s" : "%s", *chans ? chans : "aucun salon.");

	if(nptr->user)
	{
		size = count = 0;

		osntc(nick, " %s est logué sous l'username \2%s\2%s", nptr->nick, nptr->user->nick,
			IsAdmin(nptr->user) ? " (\2Admin\2)" : "");

	}
	else if((uptr = getuserinfo(parv[1]))) osntc(nick, " %s est un username enregistré. %s",
				uptr->nick, IsAdmin(uptr) ? " (\2Admin\2)" : "");

	osntc(nick, " Connecté depuis %s sur \2%s", duration(CurrentTS - nptr->ttmco), nptr->serveur->serv);
	return 1;
}
