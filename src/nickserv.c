/* src/nickserv.c - Diverses commandes sur le module nickserv
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
 * $Id: nickserv.c,v 1.209 2008/01/05 18:34:13 romexzf Exp $
 */

#include "main.h"
#include "hash.h"
#include "userinfo.h"
#include "admin_manage.h"
#include "chanserv.h"
#include "outils.h"
#include "mylog.h"
#include "cs_cmds.h"
#include "add_info.h"
#include "del_info.h"
#include "config.h"
#include "aide.h"
#include "data.h"
#include "dnr.h"

#include "nickserv.h"
#include "crypt.h"
#include "template.h"
#include <ctype.h>

static int login_report_failed(anUser *user, aNick *nick, const char *raison)
{
	if(IsAdmin(user))
	{
		const char *nuh = GetNUHbyNick(nick, 0);
		cswall("Admin Login échoué sur \2%s\2 par \2%s\2", user->nick, nuh);
		log_write(LOG_UCMD, 0, "login %s failed (%s) par %s", user->nick, raison, nuh);
	}
	return 0;
}
