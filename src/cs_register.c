/* src/cs_register.c - Enregistrement de salon
 *
 * ODreams v2 (C) 2021 -- Ext by @bugsounet <bugsounet@bugsounet.fr>
 * site web: http://www.ircdreams.org
 *
 * Services pour serveur IRC. Support� sur Ircdreams v3
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
#include "template.h"
#include "data.h"
#include "dnr.h"

/* regchan, unreg (+chancheck) + register, drop */

int chan_check(const char *chan, aNick *nick)
{
	aDNR *dnr;
	int i = 1;

	if(*chan != '#') return csreply(nick, GetReply(nick, L_CHANNELNAME));

	for(; chan[i] && i < REGCHANLEN; ++i)
		if(chan[i] == ',' || !isascii(chan[i])) /* bad char or comma */
			return csreply(nick, GetReply(nick, L_USER_INVALID), chan);
	/* if chan[i] != \0 then i > REGCHANLEN */
	if(chan[i])	return csreply(nick, GetReply(nick, L_CHANMAXLEN), REGCHANLEN);

	if((dnr = IsBadChan(chan)))
	{
		if(IsAnAdmin(nick->user))
			csreply(nick, "DNRMask: \2%s\2 Auteur: \2%s\2 Pos� le:\2 %s\2 Raison: %s",
				dnr->mask, dnr->from, get_time(nick, dnr->date), dnr->raison);
		else return csreply(nick, GetReply(nick, L_CANREGDNR), chan, dnr->raison);
	}

	return 1;
}


/*
 * register <user> <mail> <mail> [pass]
 */
int register_user(aNick *nick, aChan *chan, int parc, char **parv)
{
	anUser *user = NULL;
	char *p;
	aDNR *dnr;
	int flag = U_DEFAULT;

	if(nick->user
#ifdef ADMINREG
		&& !IsAdmin(nick->user)
#endif
	) return csreply(nick, GetReply(nick, L_LOGUEDIN), nick->user->nick);

	if(NRegister(nick) && !nick->user)
		return csreply(nick, GetReply(nick, L_CMDALREADYUSED), parv[0]);

	if(nick->user || GetConf(CF_PREMIERE|CF_NOMAIL))
	{
		if(parc < 4 || strlen(parv[4]) < 7) return csreply(nick, GetReply(nick, L_PASS_LIMIT));

		p = parv[4];
	}
	else p = create_password(nick->numeric), flag |= U_FIRST;

	if(strlen(parv[1]) > NICKLEN)
		return csreply(nick, GetReply(nick, L_USERLENLIMIT), NICKLEN);

	if(!IsValidNick(parv[1]))
		return csreply(nick, GetReply(nick, L_USER_INVALID), parv[1]);

	if((dnr = IsBadNick(parv[1])))
		return csreply(nick, GetReply(nick, L_CANREGDNR), parv[1], dnr->raison);
	/* le nick existe mais n'est pas le sien, risque d'abus */
	if(!nick->user && strcasecmp(nick->nick, parv[1]) && getnickbynick(parv[1]))
		return csreply(nick, GetReply(nick, L_UCANTREG_INUSE));

	if(getuserinfo(parv[1]))
		return csreply(nick, GetReply(nick, L_ALREADYREG), parv[1]);

	if(strcasecmp(parv[3], parv[2]))
		return csreply(nick, "Les deux emails fournis sont diff�rents, vous avez surement"
							"commis une faute de frappe.");

	if(strlen(parv[2]) > MAILLEN)
		return csreply(nick, GetReply(nick, L_MAILLENLIMIT), MAILLEN);

	if(!IsValidMail(parv[2]))
		return csreply(nick, GetReply(nick, L_MAIL_INVALID));

	if(GetUserIbyMail(parv[2])) return csreply(nick, GetReply(nick, L_MAIL_INUSE));

	user = add_regnick(parv[1], MD5pass(p, NULL), CurrentTS, CurrentTS, 1, flag, parv[2], 0);
	user->lang = DefaultLang;

	if(nick->user)
		return csreply(nick, GetReply(nick, L_ADM_USER_REGUED), user->nick, p, user->mail);

	else if(GetConf(CF_PREMIERE|CF_NOMAIL))
	{
		SetNRegister(nick);
		cs_account(nick, user);
		if(GetConf(CF_PREMIERE))
		{
			user->level = MAXADMLVL;
			chan = add_chan(bot.chan, "Salon d'aide � propos des services CoderZ");
			add_access(user, bot.chan, OWNERLEVEL, A_MANAGERFLAGS, OnChanTS(user, chan));
			csjoin(chan, JOIN_FORCE);
			db_write_chans();
			db_write_users();
			csreply(nick, "Bienvenue sur les Services ODreams " SPVERSION " !");
			csreply(nick, "Vous �tes Administrateurs des Services de niveau maximum.");
			csreply(nick, "Le salon %s vient d'�tre enregistr� sous votre Username", bot.chan);
			ConfFlag &= ~CF_PREMIERE;
		}
		else csreply(nick, GetReply(nick, L_ADM_USER_REGUED), parv[1], p, parv[2]);
		csreply(nick, "\2Vous avez �t� identifi� � TOUS les services");
		csreply(nick, "Lors de votre prochaine connexion vous pourrez vous identifier avec"
					": \2/%s %s %s %s", cs.nick, RealCmd("login"), parv[1], p);
		csreply(nick, "Le mail (%s) que vous avez fourni peut �tre utilis� pour"
					"vous envoyer un pass en cas de probl�me.", parv[2]);
		csreply(nick, "Vous pouvez voir la liste des commandes avec \2/%s %s\2",
				cs.nick, RealCmd("showcommands"));
	}
	else
	{
		if(!tmpl_mailsend(&tmpl_mail_register, user->mail, user->nick, p, nick->host))
			return csreply(nick, "Impossible d'envoyer le mail, votre pass est %s", p);

		csreply(nick, GetReply(nick, L_PASS_SENT), user->mail);
		csreply(nick, GetReply(nick, L_REGISTERTIMEOUT), duration(cf_register_timeout));
	}
	return 1;
}

int first_register(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  anUser *user = NULL;
  aNick *n;
  aHashCmd *cmdp = FindCommand("REGISTER");

  if (!(n = getnickbynick(parv[1])))
    return osntc(nick, "No such nick: %s", parv[1]);

  if(!(n->flag & N_OPER))
    return osntc(nick, "Permission Denied: You are not an IRC Operator !");

  if(!GetConf(CF_PREMIERE))
    return osntc(nick, "Reserved for the first launch!");

  if((user = getuserinfo(parv[1])))
    return osntc(nick, "%s is already registered", user->nick);

  user = add_regnick(parv[1], 10);

  osntc(nick, "Welcome to ODreams IRC Service [" SPVERSION "] !");
  osntc(nick, "You are a maximum level administrator.");

  ConfFlag &= ~CF_PREMIERE;
  cmdp->flag |= CMD_DISABLE;

  //load_cmds();
  //BuildCommandsTable(1);
  //write_cmds();
  db_write_users();

  user = getuserinfo(parv[1]);
  nick->user = user;
  nick->user->n = nick;

  return 1;
}
