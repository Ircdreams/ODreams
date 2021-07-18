/* src/aide.c - Aide
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
 * $Id: aide.c,v 1.5 2005/06/16 17:21:57 bugs Exp $
 */

#include "main.h"
#include "os_cmds.h"
#include "outils.h"
#include "debug.h"
#include "hash.h"
#include "showcommands.h"

int syntax_cmd(aNick *nick, aHashCmd *cmd)
{
	static char helpcmd[CMDLEN + 1] = {0};

	if(!*cmd->syntax) return osntc(nick, "\2Erreur de syntaxe !");
	if(helpcmd[0] == 0) strcpy(helpcmd, RealCmd("aide"));

	osntc(nick, cmd->syntax, cmd->name);
	osntc(nick, "Tapez \2/%s %s %s\2 pour plus d'informations.", os.nick, helpcmd, cmd->name);
	return 1;
}

int aide(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	char more[16], *list;
	int morelen = 0, cur = 0, i = 0;
	aHashCmd *cmdp;
	HelpBuf *help;

	if(!parc) return showcommands(nick, chaninfo, parc, parv);

	if(!(cmdp = FindCommand(parv[1]))) return osntc(nick, "La commande \2%s\2 n'existe pas.", parv[1]);

	help = cmdp->help[1];
	if(!help) return osntc(nick, "\2Aide non trouvée, consultez un Administrateur");

	if(parc > 1) {
		Strncpy(more, parv[2], sizeof more - 1);
		morelen = strlen(more);
		osntc(nick, "\2Aide\2 sur la commande \2%s\2 >> \2%s\2 (\2\003%s\2\3 Niveau %d)", parv[1], more,
			AdmCmd(cmdp) ? "4ADMIN" : ChanCmd(cmdp) ? "12CHAN" : HelpCmd(cmdp) ? "7HELPER" : "3USER", cmdp->level);
	}
	else {
		osntc(nick, "\2Aide\2 sur la commande \2%s\2 (\2\003%s\2\3 Niveau %d)", parv[1],
			AdmCmd(cmdp) ? "4ADMIN" : ChanCmd(cmdp) ? "12CHAN" : HelpCmd(cmdp) ? "7HELPER" : "3USER", cmdp->level);
		if(*cmdp->syntax) osntc(nick, cmdp->syntax, cmdp->name);
	}

	list = help->buf[help->count-1];

	for(;i < help->count;++i)
	{
		char *p = help->buf[i];
		if(*p == '|')
		{
			if(morelen && !cur)
			{
				if(strncasecmp(++p, more, morelen)) /* still not found, go on */
					continue;
				else cur = 1; /* found.. */
			}
			else break; /* found a pipe and was not looking for option or already found */
		}
		else if(morelen && !cur) continue;
		osntc(nick, "%s", p);
	}

	/* looking for option, but not found! */
	if(morelen && !cur)	osntc(nick, "Aucune aide sur \2%s\2 pour la commande %s", more, parv[1]);
	/* there are options but none found/searched */
	if(*list == '|' && !cur) osntc(nick, "Options: %s", list + 1);
	/* option found or none looked for (search success!) */
	if(cmdp->flag & CMD_DISABLE) osntc(nick, "\2\0034Note:\2\3 Commande désactivée.");

	return 1;
}

static HelpBuf *help_newbuf(void)
{
	HelpBuf *ptr = calloc(1, sizeof *ptr);
	if(!ptr) Debug(W_MAX, "help::load, newbuf failed!");

	return ptr;
}

static int help_addbuf(HelpBuf *help, const char *buf)
{
	if(!(help->buf = realloc(help->buf, sizeof *help->buf * ++help->count)))
		return Debug(W_MAX, "help::load, addbuf OOM! (%s)", buf);
	help->buf[help->count -1] = NULL;
	str_dup(&help->buf[help->count -1], buf);
	return 0;
}

static int help_read_file(FILE *fp)
{
	char buf[300], list[200];
	aHashCmd *cmdp = NULL;
	int cmds = 0,drop = 1;

	while(fgets(buf, sizeof buf, fp))
	{
		strip_newline(buf);

		if(*buf == '#') /* got a new command */
		{	/* first, end with previsous command options list, if any */
			if(cmdp && *list) help_addbuf(cmdp->help[1], list);

			if(!(cmdp = FindCoreCommand(buf + 1)))
			{
				Debug(0, "help::unknown command %s", buf + 1);
				drop = 1;
				continue;
			} /* help** has already been realloc'ed. or must be. */
			else drop = 0;

			if(cmdp->help[1])
			{
				int i = 0;
				for(;i < cmdp->help[1]->count;++i) free(cmdp->help[1]->buf[i]);
				cmdp->help[1]->count = 0;
			}
			else cmdp->help[1] = help_newbuf();
			*list = 0; /* reinit list */
			++cmds;
		}
		else if(drop) continue;
		else if(*buf == '!') Strncpy(cmdp->syntax, buf + 1, SYNTAXLEN);
		else if(*buf == '|')
		{
			char *p = strchr(buf + 1, ' ');

			if(p) help_addbuf(cmdp->help[1], buf), *p = 0;

			strcat(list, *list ? " " : "|");
			strcat(list, buf + 1);
		}
		else if(*buf) help_addbuf(cmdp->help[1], buf);
	}
	return cmds;
}

int help_load()
{
	FILE *fp;
	char buf[100];
	int i;

	snprintf(buf, sizeof buf, "aide/aide");

	if(!(fp = fopen(buf, "r")))
	{
		Debug(W_MAX, "help: impossible d'ouvrir le fichier d'aide");
		return -1; /* error */
	}

	i = help_read_file(fp);
	fclose(fp);

	if(i != CmdsCount) Debug(W_MAX, "aide: manque %d description(s)", CmdsCount - i);
	return i;
}
