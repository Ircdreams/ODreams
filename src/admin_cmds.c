/* src/admin_cmds.c - Diverses commandes pour admins
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
 * $Id: admin_cmds.c,v 1.17 2005/12/06 06:32:13 bugs Exp $
 */

#include "main.h"
#include "outils.h"
#include "os_cmds.h"
#include "fichiers.h"
#include "add_info.h"
#include "del_info.h"
#include "hash.h"
#include "config.h"
#include "divers.h"
#include "showcommands.h"
#include "aide.h"
#include "timers.h"

int inviteme(aNick *nick, aChan *chaninfo, int parc, char **parv)
{ 	/* invite des admins sur le salon d'infos */
    putserv("%s " TOKEN_INVITE " %s :%s", os.num, nick->nick, bot.pchan);
    return 1;
}

void CleanUp(void)
{
	anUser *u, *ut;
	anAccess *a, *at;
	aBAD *bad = badhead, *dt;
	aEX *ex = exhead, *et;

	int i = 0;

	free(quit_msg);
	free(pasdeperm);
	free(network);

	purge_nickandserv();

	for(i = 0;i < USERHASHSIZE;++i) for(u = user_tab[i];u;u = ut)
	{
		ut = u->next;
		for(a = u->accesshead;a;free(a), a = at)
		{
			at = a->next;
			if(a->info) free(a->info);
		}
		free(u);
	}
        for(;bad;free(bad->mask), free(bad->raison), free(bad), bad = dt) dt = bad->next;
	for(;ex;free(ex->mask), free(ex->raison), free(ex), ex = et) et = ex->next;

	return;
}

int die(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	char *r = parc ? parv2msg(parc, parv, 1, 300) : quit_msg;

	running = 0;

	putserv("%s " TOKEN_QUIT " :%s", os.num, r);
	putserv("%s " TOKEN_SQUIT " %s 0 :%s", bot.servnum, bot.server, r);

	db_write_users();

        write_cmds();
        write_trusted();
        write_gline();
	write_shun();
	write_trace();

	return 1;
}

/*
 * restart_bot parv[1->parc-1] = reason
 */
int restart_bot(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	FILE *fuptime;
	char *r = parc ? parv2msg(parc, parv, 1, 300) : quit_msg;

	if((fuptime = fopen("uptime.tmp", "w"))) /* permet de maintenir l'uptime (/me tricheur)*/
	{
		fprintf(fuptime, "%lu", bot.uptime);
		fclose(fuptime);
	}

	running = 0;
	ConfFlag |= CF_RESTART;

	putserv("%s " TOKEN_QUIT " :%s [\2Redémarrage\2]", os.num, r);
	putserv("%s "TOKEN_SQUIT" %s 0 :%s [\2RESTART\2]", bot.servnum, bot.server, r);

	db_write_users();

        write_cmds();
        write_trusted();
        write_gline();
        write_shun();
        write_trace();

	return 1;
}

int chcomname(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	aHashCmd *cmd;
	const char *newcmd = parv[2], *lastcmd = parv[1];

	if(strlen(newcmd) > CMDLEN)
		return osntc(nick, "Un nom de commande ne doit pas dépasser 14 caractères.");

	if(!(cmd = FindCommand(lastcmd)) || *cmd->name == '\1')
		return osntc(nick, "La commande \2%s\2 n'existe pas.", lastcmd);

	if(FindCommand(newcmd)) return osntc(nick, "La commande %s existe déjà.", newcmd);

	HashCmd_switch(cmd, newcmd);
	osntc(nick, "La commande\2 %s\2 s'appelle maintenant\2 %s\2.", lastcmd, cmd->name);
	BuildCommandsTable(1);
	write_cmds();
	return 1;
}

int chlevel(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	const char *cmd = parv[1];
	int level;
	aHashCmd *cmdp;

	if(!(cmdp = FindCommand(cmd))) return osntc(nick, "La commande \2%s\2 n'existe pas.", cmd);

	if(!is_num(parv[2]) || (level = atoi(parv[2])) > MAXADMLVL || level < 0)
		return osntc(nick, "Veuillez préciser un level valide.");

	if(level == cmdp->level)
		return osntc(nick, "%s est déjà au niveau %d.", cmd, level);

	if(!NeedNoAuthCmd(cmdp) && !level) 
                   return osntc(nick, "La commande %s ne peut être utilisée que par des administrateurs"
			" identifiés sur les services. Le niveau minimal est donc de 1.", cmd); 

	if(level > nick->user->level)
		return osntc(nick, "Vous ne pouvez pas donner un niveau supérieur au votre.");
	else
	{
		if(level > 1) cmdp->flag |= CMD_ADMIN;
		else cmdp->flag &= ~CMD_ADMIN;
	}

	cmdp->level = level;

	osntc(nick, "Le niveau de la commande %s est maintenant\2 %d\2 (%s).",
				cmdp->name, cmdp->level, AdmCmd(cmdp) ? "Admin" : "User");
	write_cmds();
	BuildCommandsTable(1);
	return 1;
}

int disable_cmd(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	const char *cmd = parv[1];
	int disable = 0, i = 0, nb = 0;
	aHashCmd *cmdp = FindCommand(cmd);
	aHashCmd *listcmd;

	if (!strcasecmp(cmd, "-LIST")) {

        	putserv("%s " TOKEN_NOTICE " %s :Liste des commandes désactivées:", os.num, nick->numeric);

        	for(;i < CMDHASHSIZE;++i) for(listcmd = cmd_hash[i];listcmd;listcmd = listcmd->next)
        	{
                	if(listcmd->flag & CMD_DISABLE)
                	{
                        	osntc(nick,"          Level %d - %s", listcmd->level, listcmd);
                        	nb++;
                	}
         	}
        	if(!nb)
        	osntc(nick, "Aucune commande désactivée.");
        	else
        	osntc(nick, "Fin de la liste. %d commande%s désactivée%s.",nb, nb>1 ? "s" : "", nb>1 ? "s" : "");
	}
	else {
	
		if(!cmdp) return osntc(nick, "La commande \2%s\2 n'existe pas.", cmd);

		if(!parv[2]) return 0;

		if(!strcasecmp(parv[2], "ON")) disable = 1;
		else if(strcasecmp(parv[2], "OFF")) return osntc(nick, "La valeur doit être ON ou OFF");
		if(!strcasecmp(cmdp->corename, "DISABLE")
			|| !strcasecmp(cmdp->corename, "CHCOMNAME")
			|| !strcasecmp(cmdp->corename, "CHLEVEL")
			|| !strcasecmp(cmdp->corename, "LOGIN")) /* ça nuirait à tout.. */
				return osntc(nick, "Veuillez ne pas desactiver \2%s\2.", cmd);

		if(disable) cmdp->flag |= CMD_DISABLE;
		else cmdp->flag &= ~CMD_DISABLE;

		osntc(nick, "La commande %s est maintenant \2%sactivée\2", cmd, disable ? "des" : "");
		BuildCommandsTable(1);
		write_cmds();
	}
	return 1;
}

int rehash_conf(aNick *nick, aChan *chan, int parc, char **parv)
{
	if(load_config(FICHIER_CONF) == -1)
		return osntc(nick, "Une erreur s'est produite lors de l'actualisation...");

	return osntc(nick, "Configuration actualisée avec succès");
}

int showconfig(aNick *nick, aChan *c, int parc, char **parv)
{
	osntc(nick, "Etat des variables de configuration de ODreams %s", SPVERSION);
	osntc(nick, "Nom du réseau : %s - Uplink: %s:%d", network, bot.ip, bot.port);
	osntc(nick, "Nom du serveur des Services : %s (%s) [Num: %s]", bot.server, bot.name, bot.servnum);
	osntc(nick, "Salon de controle de %s: %s", os.nick, bot.pchan);
	osntc(nick, "Message de quit : %s", quit_msg);
	osntc(nick, "Message PASDEPERM : %s", pasdeperm);
	osntc(nick, "Message de raison par defaut: %s", defraison);
	osntc(nick, "Message de block: %s", blockmsg);
	osntc(nick, "Message de silence: %s", shunmsg);
	return 1;
}

int bad_manage(aNick *nick, int type, int parc, char **parv)
{
        if(!strcasecmp(parv[1], "list"))
        {
                int from = getoption("-from", parv, parc, 2, 0);
                int mask = getoption("-match", parv, parc, 2, 0), count = 0;
                aBAD *bad = badhead;

                for(;bad;bad = bad->next)
                        if(bad->flag & type && (!from || !strcasecmp(parv[from], bad->from))
                                && (!mask || !match(parv[mask], bad->mask)))
                        {
                                osntc(nick, "BADMask: \2%s\2 Auteur: \2%s\2 Posé le:\2 %s\2 Raison: %s",
                                        bad->mask, bad->from, get_time(nick, bad->date), bad->raison);
                                ++count;
                        }
                osntc(nick, "%d matches correspondantes.", count);
        }

        else if(!strcasecmp(parv[1], "add") && parc >= 2)
        {
                aBAD *bad = find_bad(parv[2], type | BAD_MASK);
                int both = getoption("-both", parv, parc, 3, -1) ? BAD_TYPECHAN|BAD_TYPEUSER : type;

                if(bad)
                        return osntc(nick, "%s est déjà un BAD mask (Ajouté par %s, le %s [%s])",
                                bad->mask, bad->from, get_time(nick, bad->date), bad->raison);

                add_bad(parv[2], nick->user->nick,
                        parc > (both != type ? 3 : 2)
                        ? parv2msg(parc, parv, both != type ? 4 : 3, 250) : "Service",
                        CurrentTS, HasWildCard(parv[2]) ? both | BAD_MASK : both);
                osntc(nick, "%s a été ajouté aux BAD masks.", parv[2]);
                write_bad();
        }
        else if(!strcasecmp(parv[1], "del") && parc >= 2)
        {
                aBAD *bad = find_bad(parv[2], type);

                if(!bad) return osntc(nick, "%s n'est pas un BAD mask.", parv[2]);
                del_bad(bad);
                osntc(nick, "BAD mask supprimé.");
                write_bad();
        }
        else osntc(nick, "Syntaxe: %s (ADD|DEL) <bad-mask> [raison]", parv[0]);
        return 0;
}

int badchan_manage(aNick *nick, aChan *c, int parc, char **parv)
{
        return bad_manage(nick, BAD_TYPECHAN, parc, parv);
}

int baduser_manage(aNick *nick, aChan *c, int parc, char **parv)
{
        return bad_manage(nick, BAD_TYPEUSER, parc, parv);
}

int ex_manage(aNick *nick, int type, int parc, char **parv)
{
        if(!strcasecmp(parv[1], "list"))
        {
                int from = getoption("-from", parv, parc, 2, 0);
                int mask = getoption("-match", parv, parc, 2, 0), count = 0;
                aEX *ex = exhead;

                for(;ex;ex = ex->next)
                        if(ex->flag & type && (!from || !strcasecmp(parv[from], ex->from))
                                && (!mask || !match(parv[mask], ex->mask)))
                        {
                                osntc(nick, "EXMask: \2%s\2 Auteur: \2%s\2 Posé le:\2 %s\2 Raison: %s",
                                        ex->mask, ex->from, get_time(nick, ex->date), ex->raison);
                                ++count;
                        }
                osntc(nick, "%d matches correspondantes.", count);
        }

        else if(!strcasecmp(parv[1], "add") && parc >= 2)
        {
                aEX *ex = find_ex(parv[2], type | EX_MASK);
                int both = getoption("-both", parv, parc, 3, -1) ? EX_TYPECHAN|EX_TYPEUSER : type;

                if(ex)
                        return osntc(nick, "%s est déjà un EX mask (Ajouté par %s, le %s [%s])",
                                ex->mask, ex->from, get_time(nick, ex->date), ex->raison);

                add_ex(parv[2], nick->user->nick,
                        parc > (both != type ? 3 : 2)
                        ? parv2msg(parc, parv, both != type ? 4 : 3, 250) : "Service",
                        CurrentTS, HasWildCard(parv[2]) ? both | EX_MASK : both);
                osntc(nick, "%s a été ajouté aux EX masks.", parv[2]);
                write_ex();
        }
        else if(!strcasecmp(parv[1], "del") && parc >= 2)
        {
                aEX *ex = find_ex(parv[2], type);

                if(!ex) return osntc(nick, "%s n'est pas un EX mask.", parv[2]);
                del_ex(ex);
                osntc(nick, "EX mask supprimé.");
                write_ex();
        }
        else osntc(nick, "Syntaxe: %s (ADD|DEL) <ex-mask> [raison]", parv[0]);
        return 0;
}

int exchan_manage(aNick *nick, aChan *c, int parc, char **parv)
{
        return ex_manage(nick, EX_TYPECHAN, parc, parv);
}

int exuser_manage(aNick *nick, aChan *c, int parc, char **parv)
{
        return ex_manage(nick, EX_TYPEUSER, parc, parv);
}
