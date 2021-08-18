/* src/chaninfo.c - Affiche les informations à propos d'un salon
 *
 * Copyright (C) 2002-2008 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@coderz.info>
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
 * $Id: chaninfo.c,v 1.51 2008/01/04 13:21:34 romexzf Exp $
 */

#include "main.h"
#include "outils.h"
#include "ban.h"
#include "admin_chan.h"
#include "cs_cmds.h"
#include "hash.h"
#include "data.h"
#include "chaninfo.h"

static char *GetChanOptions(aChan *chan)
{
	static char coptions[10+6+9+7+9+11+7+7+8+14+20+1];
	int i = 0;
	coptions[0] = 0;

	if(CLockTopic(chan)) strcpy(coptions, " locktopic"), i += 10;
	if(CNoOps(chan)) strcpy(coptions + i, " noops"), i += 6;
	if(CNoVoices(chan)) strcpy(coptions + i, " novoices"), i += 9;
	if(CNoBans(chan)) strcpy(coptions + i, " nobans"), i += 7;
	if(CStrictOp(chan)) strcpy(coptions + i, " strictop"), i += 9;
	if(CAutoInvite(chan)) strcpy(coptions + i, " autoinvite"), i += 11;
	if(CWarned(chan)) strcpy(coptions + i, " warned"), i += 7;
	if(CNoInfo(chan)) strcpy(coptions + i, " noinfo"), i += 7;
	if(chan->flag & C_ALREADYRENAME) strcpy(coptions + i, " renommé"), i += 8;
	if(CFLimit(chan)) mysnprintf(coptions + i, sizeof coptions - i, " auto-limit(%u,%u)",
						chan->limit_inc, chan->limit_min);
	if(coptions[0] == 0) strcpy(coptions, " Aucune");

	return coptions;
}
