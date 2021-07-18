/* src/timers.c - fonctions de controle appellées periodiquement
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
 * $Id: timers.c,v 1.6 2005/05/15 23:31:52 bugs Exp $
 */

#include "main.h"
#include "config.h"
#include "outils.h"
#include "hash.h"
#include "del_info.h"
#include "fichiers.h"
#include "os_cmds.h"
#include "debug.h"

void check_glines(void)
{
	struct gline *tmp = glinehead;
	time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);
	if(glinehead)
	{
        	for(;tmp;tmp = tmp->next)
			if(tmp->time < CurrentTS)
			{
			oswallops("Expiration du GLINE: %s", tmp->host);
			putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] < \2\0037UNGLINE\2\3 %s par %s",
                                        os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                                                tmp->host, os.nick);
			del_gline(tmp->host);
			write_gline();
			}
	}        
}

void check_shun(void)
{
        struct shun *tmp = shunhead;
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

        if(shunhead)
        {
                for(;tmp;tmp = tmp->next)
                        if(tmp->time < CurrentTS)
                        {
                        	oswallops("Expiration du SHUN: %s", tmp->host);
	                       		putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] < \2\0037UNSHUN\2\3 %s par %s",
                                        	os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                                                	tmp->host, os.nick);
                        	del_shun(tmp->host);
                        	write_shun();
                        }
        }
}

void exec_timers(void) 
{ 
        bot.lasttime = CurrentTS;       /* mise à jour des infos  */ 
        bot.lastbytes = bot.dataQ;      /* pour les stats traffic */ 
    
	check_glines();
	check_shun();
}
