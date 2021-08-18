/* src/opdeop.c - Fonctions de base chanserv (op, deop, voice, devoice, etc)
 *
 * Copyright (C) 2002-2006 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@ir3.org>
 *                         Benjamin Beret <kouak@kouak.org>
 *
 * site web: http://sf.net/projects/scoderz/
 *
 * Services pour serveur IRC. Supporté sur IRCoderz
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
 * $Id: opdeop.c,v 1.33 2006/02/26 16:53:54 romexzf Exp $
 */

#include "main.h"
#include "opdeop.h"
#include "cs_cmds.h"
#include "hash.h"

static int massmode_add(aJoin *join, aNick *nick, char *numbuf, char *modebuf,
	int *count, int sign, unsigned int mode)
{	/* SSCC1 SSCC2 SSCC3*/
	if(sign == '+' && !(join->status & mode)) join->status |= mode;
	else if(sign == '-' && join->status & mode) join->status &= ~mode;
	else return *count;

	modebuf[*count] = (mode == J_OP) ? 'o' : 'v'; /* ajout du mode */
	modebuf[*count+1] = 0;

	numbuf[*count * NUMLEN] = ' ';
	strcpy(numbuf + *count*NUMLEN + 1, nick->numeric); /* Num + espace ajouté */

	if(++*count > 5)
	{
		csmode(join->chan->regchan, MODE_OBV, sign == '+' ? "+$$" : "-$$", modebuf, numbuf);
		*count = 0;
		*modebuf = *numbuf = 0;
	}
	return *count;
}

int voiceall(aNick *nick, aChan *chan, int parc, char **parv)
{
	aLink *lp = chan->netchan->members;
	char buf[60] = {0}, m[7] = {0};
	int reste = 0, count = 0;

	if(CNoVoices(chan)) return csreply(nick, "Le NOVOICES est actif sur %s", chan->chan);

	for(; lp; lp = lp->next)
		if(!IsVoice(lp->value.j))
			reste = massmode_add(lp->value.j, lp->value.j->nick, buf, m, &count, '+', J_VOICE);

	if(reste) csmode(chan, MODE_OBV, "+$ $", m, buf);
	return 1;
}

int devoiceall(aNick *nick, aChan *chan, int parc, char **parv)
{
	aLink *lp = chan->netchan->members;
	char buf[60] = {0}, m[7] = {0};
	int reste = 0, count = 0;

	for(; lp; lp = lp->next)
		if(IsVoice(lp->value.j))
			reste = massmode_add(lp->value.j, lp->value.j->nick, buf, m, &count, '-', J_VOICE);

	if(reste) csmode(chan, MODE_OBV, "-$ $", m, buf);
	return 1;
}

