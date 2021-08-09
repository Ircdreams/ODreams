/* src/showcommands.c - Liste les commandes
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
 * $Id: showcommands.c,v 1.8 2005/06/16 17:21:57 bugs Exp $
 */

#include "main.h"
#include "outils.h"
#include "os_cmds.h"
#include "showcommands.h"
#include "hash.h"

static struct scmd_cmds {
	int level;
	int nb;
	char (*buf)[100];
} scmd_user[MAXADMLVL + 1];

/* 	BuildCommandsTable(): construit deux tableaux contenant pour
 *		chaque level (struct.level) la liste des commandes correspondantes
 *		struct.nb contient le nombre d'élement du tableau struct.buf
 *		while(i < struct.nb) printf("level=%d %s\n", struct.level, struct.buf[i++]);
 * => appelée une fois au démarrage, permet des appels 8-10 fois plus rapide
 * => de showcommands puisque le triage n'est plus à faire. -Cesar
 */

int BuildCommandsTable(int rebuild)
{
	int i = 0, level = 0;
	aHashCmd *cmd;
	struct scmd_cmds *tmp = NULL;

	if(rebuild) { /* reconstruction de la table au runtime, nettoyage de l'ancienne */
		for(i = 0;i < 11;++i) free(scmd_user[i].buf), scmd_user[i].buf = NULL;
	}

	for(i = 0;i < CMDHASHSIZE;++i) for(cmd = cmd_hash[i];cmd;cmd = cmd->next)
	{
		level = cmd->level;

		if(ChanCmd(cmd) || (ChanCmd(cmd) && !AdmCmd(cmd))
			|| *cmd->corename == CTCP_DELIM_CHAR || cmd->flag & CMD_DISABLE) continue;

		tmp = &scmd_user[level];
		if(!tmp->buf)/* premiere commande de ce level */
		{/* preparons les buffers */
			tmp->nb = 1;
			tmp->buf = malloc(sizeof *tmp->buf);
			tmp->level = sprintf(*tmp->buf, "\2\0033Level %3d:\2\3", level);
		}
		else if(tmp->level > 85)/* ligne pleine, ajoutons une nouvelle */
		{
			tmp->buf = realloc(tmp->buf, sizeof *tmp->buf * ++tmp->nb);
			strcpy(tmp->buf[tmp->nb-1], "          \2\0033:\3\2");
			tmp->level = 16;
		}
		/* ajout de la commande dans les buffers */
		tmp->level += fastfmt(tmp->buf[tmp->nb-1] + tmp->level, " $", cmd->name);

	}/* for cmds */
	return 0;
}

int showcommands(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	int i, j;

	osntc(nick, "Commandes \2User\2");
	for(i = 0;i <= (nick->user ? nick->user->level : 0);++i)
	{
		if(i == ADMINLEVEL) osntc(nick, "Commandes \2Administrateur\2");
		for(j = 0; j < scmd_user[i].nb;++j)
    		osntc(nick, "%s", scmd_user[i].buf[j]);

	}

	osntc(nick, "Pour de l'aide sur une commande tapez \2/%s %s <commande>\2", os.nick, RealCmd("aide"));
	return 1;
}
