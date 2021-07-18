/* src/admin_manage.c - commandes pr gerer les admins
 * Copyright (C) 2004-2005 ircdreams.org
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
 * $Id: admin_manage.c,v 1.4 2005/05/22 15:58:51 bugs Exp $
 */

#include "main.h"
#include "add_info.h"
#include "outils.h"
#include "os_cmds.h"
#include "fichiers.h"
#include "hash.h"

aNick **adminlist = NULL; 
int adminmax = 0; 
    
/* Try to insert a new admin in list, otherwise append it. */ 
int adm_active_add(aNick *nick) 
{ 
        int i = 0; 
 
        while(i < adminmax && adminlist[i]) ++i; /* look for a free slot */ 
        /* can't find one, make the list grow up */ 
        if(i >= adminmax) adminlist = realloc(adminlist, sizeof *adminlist * ++adminmax); 
        adminlist[i] = nick; /* insert it into the free slot */ 
        return adminmax; 
} 
    
/* Try to find admin in list and empties the slot */ 
int adm_active_del(aNick *nick) 
{ 
        int i = 0; 
    
        while(i < adminmax && adminlist[i] != nick) ++i; /* searching in list.. */ 
        if(i < adminmax) adminlist[i] = NULL; /* free the slot */ 
        return 0; 
}
