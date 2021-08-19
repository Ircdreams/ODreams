/* src/fichiers.c - Lecture/Écriture des données
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
 * $Id: fichiers.c,v 1.131 2008/01/05 18:34:13 romexzf Exp $
 */

#include "main.h"
#include "fichiers.h"
#include "outils.h"
#include "config.h"
#include "hash.h"
#include "debug.h"
#include "mylog.h"
#include "cs_cmds.h"
#include "add_info.h"
#include "data.h"

static void db_parse_data(aData **dpp, void *data, int flag, char **ar)
{ /* <DATA> <from> <fin> <debut> :<raison> */
	long int fin = strtol(ar[2], NULL, 10);
	long int debut = strtol(ar[3], NULL, 10);

	data_load(dpp, ar[1], ar[4], flag, fin > 0 ? fin - debut : fin, debut, data);
}

int db_load_users(int quiet)
{
	FILE *fp;
	char buf[500], *ar[20];
	int adm = 0, count = 0, line = 0;
	anUser *u = NULL;
	anAccess *a;

	if(!(fp = fopen(DBDIR "/" DBUSERS, "r")))
	{
		ConfFlag |= CF_PREMIERE;
		return 0;
	}

	while(fgets(buf, sizeof buf, fp))
	{
		int items = split_buf(buf, ar, ASIZE(ar));
		++line;
		if(items < 2) continue;

		strip_newline(ar[items-1]); /* remove trailing \r\n */
		/* NICK <user> <lvl> <lang> */
		if(!strcmp(buf, "NICK"))
		{
			char *nick = ar[1], *lang = ar[3];
			int level = strtol(ar[2], NULL, 10);

			u = NULL; /* reset */

			if(items < 4)
			{
				log_write(LOG_DB, LOG_DOTTY, "users::load(%s) item complet", ar[1]);
				continue;
			}

			if(!(u = add_regnick(nick, level)))
			{
				log_write(LOG_DB, LOG_DOTTY, "users::load(%s) erreur interne", nick);
				continue;
			}
			++count;
			if(u->level >= ADMINLEVEL) ++adm; /* at least one admin in DB ? */
			/* if lang was not available anymore, set to default */
			if(!(u->lang = lang_isloaded(lang))) u->lang = DefaultLang;
		}

		else if(!strcmp(buf, "V"))
		{	/* restore maxid in use */
			if(ar[2]) user_maxid = strtoul(ar[2], NULL, 10);
		}

		else if(u) log_write(LOG_DB, LOG_DOTTY, "users::load:%d: unknown/invalid data %s [%s]",
						line, buf, ar[1]);
	}
	fclose(fp);
	if(!adm) log_write(LOG_DB, LOG_DOTTY, "Aucun Admin localisé dans la Base de Données!");
	if(!quiet) printf("Base de donnée User chargée (%d)\n", count);
	return count;
}

int db_write_users(void)
{
	anAccess *a;
	anUser *u;

 	int i = 0;
 	FILE *fp;

	if(rename(DBDIR "/" DBUSERS, DBDIR "/" DBUSERS ".back") < 0 && errno != ENOENT)
		return log_write(LOG_DB, LOG_DOWALLOPS, "users::write: rename() failed: %s",
					strerror(errno));

	if(!(fp = fopen(DBDIR "/" DBUSERS, "w")))
		return log_write(LOG_DB, LOG_DOWALLOPS, "users::write: fopen() failed: %s",
					strerror(errno));

	fprintf(fp, "V %d %lu\n", DBVERSION_U, user_maxid);

	for(; i < USERHASHSIZE; ++i) for(u = user_tab[i]; u; u = u->next)
	{
		fprintf(fp, "NICK %s %d %s\n",
			u->nick, u->level, u->lang->langue);
	}

    fclose(fp);
    return 1;
}

void write_cmds(void)
{
	FILE *fp = fopen(CMDS_FILE, "w");
	int i = 0;
	aHashCmd *cmd;

	if(!fp)
	{
		log_write(LOG_DB, LOG_DOWALLOPS, "cmds::write: fopen() failed: %s", strerror(errno));
		return;
	}

	for(; i < CMDHASHSIZE; ++i) for(cmd = cmd_hash[i]; cmd; cmd = cmd->next)
	{
		if(!IsCTCP(cmd)) fprintf(fp, "%s %s %d %d %d\n", cmd->name,
			cmd->corename, cmd->level, cmd->flag, cmd->used);
	}
	fclose(fp);
}

int load_cmds(int quiet)
{
	char buf[80], *ar[5];
	int count = 0;
	aHashCmd *cmd;
	FILE *fp = fopen(CMDS_FILE, "r");

	if(!fp) return log_write(LOG_DB, LOG_DOTTY, "cmds::load: fopen() failed: %s", strerror(errno));

	while(fgets(buf, sizeof buf, fp) && split_buf(buf, ar, ASIZE(ar)) == 5)
	{
		int level = strtol(ar[2], NULL, 10), flag = strtol(ar[3], NULL, 10);

		if(!(cmd = FindCommand(ar[1])))/* recherche sur corename, mais c'est au load */
		{							   	/* donc corename == name */
			log_write(LOG_DB, LOG_DOTTY, "cmds::load: commande %s non trouvé dans la hash (%s)",
				ar[1], ar[0]);
			continue;
		}
		/* cmd core différente de celle du network? */
		if(strcasecmp(cmd->name, ar[0])) HashCmd_switch(cmd, ar[0]);

		if(level || NeedNoAuthCmd(cmd)) cmd->level = level;
		/* ok.. set flag to the saved ones plus internal ones .. */
		cmd->flag = (flag & ~CMD_INTERNAL) | (cmd->flag & CMD_INTERNAL);
		cmd->used = strtol(ar[4], NULL, 10);
		++count;
	}
	fclose(fp);
	if(!quiet) printf("Chargement des commandes IRC... OK (%d)\n", count);
	return count;
}

int write_files(aNick *nick, aChan *chan, int parc, char **parv)
{
	int write = 0;

	if(getoption("-user", parv, parc, 1, GOPT_FLAG)) write |= 0x1;

	switch(write)
	{
		case 0:
			write_cmds();
		case 0x1:
			db_write_users();
			break;
	}
	return csntc(nick, "Les fichiers ont bien été écrits.");
}
