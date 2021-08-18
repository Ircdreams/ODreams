/* src/moduser.c - Modification des paramètres d'un accès
 *
 * Copyright (C) 2002-2005 David Cortier  <Cesar@irc.jeux.fr>
 *                         Romain Bignon  <Progs@kouak.org>
 *                         Benjamin Beret <kouak@kouak.org>
 *
 * site web: http://sf.net/projects/scoderz/
 *
 * Services pour serveur IRC. Supporté sur IrcProgs et IRCoderz
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
 * $Id: moduser.c,v 1.36 2005/09/26 23:04:26 romexzf Exp $
 */

#include "main.h"
#include "outils.h"
#include "cs_cmds.h"
#include "hash.h"
