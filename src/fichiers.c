/* src/fichiers.c - Lecture/Écriture des données
 * Copyright (C) 2002-2005 Inter System
 *
 * contact: Progs@Inter-System.Net
 *          Cesar@Inter-System.Net
 *          kouak@kouak.org
 * site web: http://coderz.inter-system.net
 *
 * Services pour serveur IRC. Supporté sur IrcProgs et IrCoderZ
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
 * $Id: fichiers.c,v 1.9 2005/06/16 17:21:57 bugs Exp $
 */

#include <errno.h>
#include "main.h"
#include "fichiers.h"
#include "outils.h"
#include "config.h"
#include "hash.h"
#include "debug.h"
#include "os_cmds.h"
#include "add_info.h"

int db_load_users(void)
{
	FILE *fp;
	char buf[500], *ar[20];
	int adm = 0, count = 0, line = 0;
	anUser *u = NULL;

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

		if(!strcmp(buf, "NICK"))
		{ 
			char *nick = ar[1];
			int level = strtol(ar[2], NULL, 10);
			u = NULL; /* reset */

			if(items < 3)
			{
				Debug(W_TTY, "Erreur lors de la lecture de la DB pour %s, item incomplet", ar[1]);
				continue;
			}

			if(!(u = add_regnick(nick, level)))
			{
				Debug(W_TTY, "Erreur lors de la lecture de la DB pour %s, ierreur interne", nick);
				continue;
			}
			++count;
			if(u->level >= ADMINLEVEL) ++adm; /* at least one admin in DB ? */
		}
		else if(strcmp(buf, "V") && u)
				Debug(W_TTY, "DB::load: unknown/invalid data %s [%s] line %d", buf, ar[1], line);
	}
	fclose(fp);
	if(!adm) Debug(W_TTY, "Aucun Admin localisé dans la Base de Données!");
	return count;
}

int db_write_users(void)
{
	anUser *u;
 	int i = 0;
 	FILE *fp;

	if(rename(DBDIR "/" DBUSERS, DBDIR "/" DBUSERS ".back") < 0 && errno != ENOENT)
			return Debug(W_MAX|W_WARN, "DB::Write: rename() failed: %s", strerror(errno));

	if(!(fp = fopen(DBDIR "/" DBUSERS, "w")))
		return Debug(W_MAX|W_WARN, "DB::Write: Impossible d'écrire la DB user: %s",
						strerror(errno));

	fprintf(fp, "V %d\n", DBVERSION_U);

	for(;i < USERHASHSIZE;++i) for(u = user_tab[i];u;u = u->next)
	{
		fprintf(fp, "NICK %s %d\n", u->nick, u->level);
	}
    fclose(fp);
    return 1;
}

void write_trusted(void)
{
	FILE *fp = fopen(TRUSTED_FILE, "w");
        struct trusted *trust = trusthead;

        if(!fp) return;

        for(;trust;trust = trust->next)
                fprintf(fp, "%s\n", trust->ip);

        fclose(fp);
        return;
}

int load_trusted(void)
{
   char buff[300];
   int stats = 0;
   FILE *fp = fopen(TRUSTED_FILE, "r");

   if(!fp) return Debug(W_TTY, "DB-load: Erreur lors de la lecture des trusted: %s", strerror(errno));
   while(fgets(buff, sizeof buff, fp))
   {
           strip_newline(buff);
           add_trusted(buff);
	   stats++;
   }
   fclose(fp);
   return stats;
}

int load_trace(void)
{
   char buff[512], *mask, *raison= NULL;
   int stats = 0;
   FILE *fp = fopen(TRACE_FILE, "r");

   if(!fp) return Debug(W_TTY, "DB-load: Erreur lors de la lecture de trace: %s", strerror(errno));
   while(fgets(buff, sizeof buff, fp))
   {
           strip_newline(buff);
           mask = strtok(buff, " ");
           raison = strtok(NULL, ":");

           add_trace(mask,raison);
           stats++;
   }
   fclose(fp);
   return stats;
}


void write_trace(void)
{
        FILE *fp = fopen(TRACE_FILE, "w");
        struct trace *trace = tracehead;

        if(!fp) return;

        for(;trace;trace = trace->next)
                fprintf(fp, "%s :%s\n", trace->mask, trace->raison);

        fclose(fp);
        return;
}

void write_gline(void)
{
        FILE *fp = fopen(GLINE_FILE, "w");
        struct gline *gline = glinehead;

        if(!fp) return;

        for(;gline;gline = gline->next)
                fprintf(fp, "%s %d :%s\n", gline->host, gline->time, gline->raison);

        fclose(fp);
        return;
}

void write_shun(void)
{
        FILE *fp = fopen(SHUN_FILE, "w");
        struct shun *shun = shunhead;

        if(!fp) return;

        for(;shun;shun = shun->next)
                fprintf(fp, "%s %d :%s\n", shun->host, shun->time, shun->raison);

        fclose(fp);
        return;
}

int load_gline(void)
{
   char buff[512], *host, *raison= NULL;
   int stats = 0, time =0, tmp =0;
   FILE *fp = fopen(GLINE_FILE, "r");

   if(!fp) return Debug(W_TTY, "DB-load: Erreur lors de la lecture des glines: %s", strerror(errno));
   while(fgets(buff, sizeof buff, fp))
   {
           strip_newline(buff);
           host = strtok(buff, " ");
           time = atoi(strtok(NULL, " "));
           raison = strtok(NULL, ":");

	   if (time > CurrentTS)
	   {
		tmp = time - CurrentTS;
		putserv("%s GL * +%s %d :%s [Expire %s]", bot.servnum, host, tmp, raison, 
			 get_time(NULL,time));
           	add_gline(host, time, raison);
           	stats++;
	   }
   }
   fclose(fp);
   return stats;
}

int load_shun(void)
{
   char buff[512], *host, *raison= NULL;
   int stats = 0, time =0, tmp =0;
   FILE *fp = fopen(SHUN_FILE, "r");

   if(!fp) return Debug(W_TTY, "DB-load: Erreur lors de la lecture des shuns: %s", strerror(errno));
   while(fgets(buff, sizeof buff, fp))
   {
           strip_newline(buff);
           host = strtok(buff, " ");
           time = atoi(strtok(NULL, " "));
           raison = strtok(NULL, ":");

           if (time > CurrentTS)
           {
                tmp = time - CurrentTS;
                putserv("%s SU * +%s %d :%s [Expire %s]", bot.servnum, host, tmp, raison,
                         get_time(NULL,time));

                add_shun(host, time, raison);
                stats++;
           }
   }
   fclose(fp);
   return stats;
}

void write_cmds(void)
{
	FILE *fp = fopen(CMDS_FILE, "w");
	int i = 0;
	aHashCmd *cmd;
	if(!fp) return;

	for(;i < CMDHASHSIZE;++i) for(cmd = cmd_hash[i];cmd;cmd = cmd->next)
	{
		if(*cmd->name != '\001') fprintf(fp, "%s %s %d %d %d\n", cmd->name,
			cmd->corename, cmd->level, cmd->flag, cmd->used);
	}
	fclose(fp);
}

int load_cmds(void)
{
	char buff[80], name[CMDLEN + 1], corename[CMDLEN + 1];
	int i = 0;
	aHashCmd *cmd;
	FILE *fp = fopen(CMDS_FILE, "r");

	if(!fp) return Debug(W_TTY, "DB::load: Erreur lors de la lecture des commandes: %s", strerror(errno));
	while(fgets(buff, sizeof buff, fp))
	{
		int level = 0, flag = 0, used = 0;
		sscanf(buff, "%14s %14s %d %d %d", name, corename, &level, &flag, &used);

		if(!(cmd = FindCommand(corename)))/* recherche sur corename, mais c'est au load*/
		{							/* donc corename == name*/
			Debug(W_TTY, "load_cmds: commande %s non trouvée dans la hash (%s)", corename, name);
			continue;
		}
		/* cmd core différente de celle du network?*/
		if(strcasecmp(cmd->name, name))	HashCmd_switch(cmd, name);
		
		if(level || NeedNoAuthCmd(cmd)) cmd->level = level;
		/* ok.. set flag to the saved ones plus internal ones .. */
		cmd->flag = (flag & ~CMD_INTERNAL) | (cmd->flag & CMD_INTERNAL);
		cmd->used = used;
		i++;
	}
	fclose(fp);
	return i;
}

int load_bad(void)
{
        char buf[512], *ar[5];
        int count = 0, line = 0;
        FILE *fp = fopen(BAD_FILE, "r");

        if(!fp) return Debug(W_TTY, "DB::load: Erreur lors de la lecture des bad: %s",
                                                strerror(errno));

        while(fgets(buf, sizeof buf, fp))
        {
                strip_newline(buf);
                ++line;

                if(split_buf(buf, ar, ASIZE(ar)) > 4 && add_bad(*ar, ar[1],     ar[4],
                        strtol(ar[3], NULL, 10), strtoul(ar[2], NULL, 10))) ++count;
                else Debug(W_MAX|W_TTY, "load_bad: malformed BAD line %d (%s)", line, *ar);
        }
        fclose(fp);

        return count;
}

int load_ex(void)
{
        char buf[512], *ar[5];
        int count = 0, line = 0;
        FILE *fp = fopen(EX_FILE, "r");

        if(!fp) return Debug(W_TTY, "DB::load: Erreur lors de la lecture des ex: %s",
                                                strerror(errno));

        while(fgets(buf, sizeof buf, fp))
        {
                strip_newline(buf);
                ++line;

                if(split_buf(buf, ar, ASIZE(ar)) > 4 && add_ex(*ar, ar[1],     ar[4],
                        strtol(ar[3], NULL, 10), strtoul(ar[2], NULL, 10))) ++count;
                else Debug(W_MAX|W_TTY, "load_ex: malformed EX line %d (%s)", line, *ar);
        }
        fclose(fp);

        return count;
}

void write_bad(void)
{
        FILE *fp = fopen(BAD_FILE, "w");
        aBAD *tmp = badhead;

        for(;tmp;tmp = tmp->next) fprintf(fp, "%s %s %u %ld :%s\n",tmp->mask, tmp->from,
                                                                        tmp->flag, tmp->date, tmp->raison);
        fclose(fp);
}

void write_ex(void)
{
        FILE *fp = fopen(EX_FILE, "w");
        aEX *tmp = exhead;

        for(;tmp;tmp = tmp->next) fprintf(fp, "%s %s %u %ld :%s\n",tmp->mask, tmp->from,
                                                                        tmp->flag, tmp->date, tmp->raison);
        fclose(fp);
}

int write_files(aNick *nick, aChan *chan, int parc, char **parv)
{

	db_write_users();
	write_cmds();
        write_trusted();
        write_gline();
        write_shun();
        write_bad();
        write_ex();
        write_trace();
        osntc(nick, "Les fichiers ont bien été écrits.");
        return 1;
}

void append_file(const char *file, const char *ligne)
{
        FILE *fp = fopen(file, "a");
        if(fp)
        {
                fputs(ligne, fp);
                fputc('\n', fp);
                fclose(fp);
        }
        else Debug(W_WARN|W_MAX, "Impossible d'écrire [%s] dans '%s' (%s)", ligne, file, strerror(errno));
        return;
}
