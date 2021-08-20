/* src/add_info.c - Ajout d'informations en mémoire
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
 * $Id: add_info.c,v 1.11 2005/07/18 04:47:46 bugs Exp $
 */

#include "main.h"
#include "debug.h"
#include "outils.h"
#include "config.h"
#include "hash.h"
#include "add_info.h"
#include "os_cmds.h"
#include "del_info.h"
#include "serveur.h"

int add_server(const char *name, const char *num, const char *hop, const char *proto, const char *from) 
{ 
  aServer *links = calloc(1, sizeof *links);
  struct gline *gline = glinehead;
  struct shun *shun = shunhead;

  unsigned int servindex;

  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);
  putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] > \2\0037NETMERGE\2\3 %s", os.num, bot.pchan, ntime->tm_hour,
    ntime->tm_min, ntime->tm_sec, name);

  if(!links)
    return Debug(W_MAX|W_WARN, "m_server, malloc() a échoué pour le Link %s", name);

  Strncpy(links->serv, name, HOSTLEN);
  Strncpy(links->num, num, NUMSERV);
  links->maxusers = base64toint(num + NUMSERV);
  servindex = base64toint(links->num);

  for(links->smask = 16;links->smask < links->maxusers; links->smask <<= 1);
  links->smask--;/* on passe de 010000 à 001111 */

  if(serv_tab[servindex])
    return Debug(W_DESYNCH|W_WARN, "m_server: collision, le serveur %s[%s] existe déjà?", name, links->num);

  links->hub = num2servinfo(from); /* ptr vers son Hub */
  serv_tab[servindex] = links;

  if(!(num_tab[servindex] = calloc(1, (links->maxusers+1) * sizeof(aNick *))))
    return Debug(W_MAX|W_WARN, "m_server, malloc() a échoué pour les %d Offsets de %s", links->maxusers+1, name);

  /* 1erhub = my uplink -> je suis enregistré! */
  if(atoi(hop) == 1 && strcasecmp(name, bot.server)) mainhub = links;

  links->flag = (*proto == 'J') ? ST_BURST : ST_ONLINE;

  /* nouveau link on ajoute les glines / shuns sur le nouveau serveur link */

  if(glinehead) {
    for(;gline;gline = gline->next)
    {
      if (gline->time > CurrentTS)
      {
        putserv("%s GL * +%s %d :%s [Expire %s]", bot.servnum, gline->host, gline->time - CurrentTS, gline->raison,
        get_time(NULL,gline->time));
      }
    }
  }

  if(shunhead) {
    for(;shun;shun = shun->next)
    {
      if (shun->time > CurrentTS)
      {
        putserv("%s SU * +%s %d :%s [Expire %s]", bot.servnum, shun->host, shun->time - CurrentTS, shun->raison,
        get_time(NULL,shun->time));
      }
    }
  }

	return 0; 
} 

void add_trusted(const char *ip)
{
  struct trusted *ptr = malloc(sizeof *ptr);

  if(!ptr) {
    Debug(W_MAX, "add_trusted, malloc a échoué pour l'ip %s", ip);
    return;
  }

  Strncpy(ptr->ip, ip, 15);
  ptr->next = trusthead;
  trusthead = ptr;
}

void add_gline(const char *host, int time, const char *raison)
{
  struct gline *ptr = malloc(sizeof *ptr);
  struct gline *tmp, *save;

  if(glinehead) {
    for(tmp = glinehead;tmp;tmp = tmp->next) {
      save = tmp->next;
      if (!strcmp(tmp->host,host)) del_gline(tmp->host);
    }
  }

  if(!ptr) {
    Debug(W_MAX, "add_gline, malloc a échoué pour le host %s", host);
    return;
  }

  Strncpy(ptr->host, host, HOSTLEN);
  ptr->time = time;
  Strncpy(ptr->raison, raison, sizeof ptr->raison -1);
  ptr->next = glinehead;
  glinehead = ptr;
}

void add_trace(const char *mask, const char *raison)
{
  struct trace *ptr = malloc(sizeof *ptr);
  struct trace *tmp, *save;

  if(tracehead) {
    for(tmp = tracehead;tmp;tmp = tmp->next) {
      save = tmp->next;
      if (!strcmp(tmp->mask,mask)) del_trace(tmp->mask);
    }
  }

  if(!ptr) {
    Debug(W_MAX, "add_trace, malloc a échoué pour le mask %s", mask);
    return;
  }
  Strncpy(ptr->mask, mask, HOSTLEN);
  Strncpy(ptr->raison, raison, sizeof ptr->raison -1);
  ptr->next = tracehead;
  tracehead = ptr;
}

void add_shun(const char *host, int time, const char *raison)
{
  struct shun *ptr = malloc(sizeof *ptr);
  struct shun *tmp, *save;

  if(shunhead) {
    for(tmp = shunhead;tmp;tmp = tmp->next) {
      save = tmp->next;
      if (!strcmp(tmp->host,host)) del_shun(tmp->host);
    }
  }

  if(!ptr) {
    Debug(W_MAX, "add_shun, malloc a échoué pour le host %s", host);
    return;
  }

  Strncpy(ptr->host, host, HOSTLEN);
  ptr->time = time;
  Strncpy(ptr->raison, raison, sizeof ptr->raison -1);
  ptr->next = shunhead;
  shunhead = ptr;
}

void add_join(aNick *nick, const char *chan, int status, time_t timestamp, aChan *c)
{
	aJoin *join = calloc(1, sizeof *join);

	if(!join)
	{
		Debug(W_MAX|W_WARN, "add_join, malloc a échoué pour aJoin de %s sur %s", nick->nick, chan);
		return;
	}

	Strncpy(join->channel, chan, CHANLEN);
	join->status = (status & ~J_BURST);
	join->timestamp = timestamp;
	join->nick = nick;
	join->next = nick->joinhead; /* insert at top of the linked list */
	nick->joinhead = join;

	if(status & J_BURST) return; /* it's a Burst, nothing more to do.. */

	/* Perform priviledged(access-ed) user checks */

	if(nick->flag & N_HIDE) return;        

	return;
}

aBAD *add_bad(const char *mask, const char *from, const char *raison,
        time_t date, unsigned int flag)
{
  aBAD *bad = calloc(1, sizeof *bad);

  if(!bad)
  {
    Debug(W_MAX|W_WARN, "add_bad: OOM for %s[%s] (%s)", mask, from, raison);
    return NULL;
  }

  str_dup(&bad->mask, mask);
  str_dup(&bad->raison, raison);
  Strncpy(bad->from, from, NICKLEN);
  bad->date = date;
  bad->flag = flag;
  bad->next = badhead;
  if(badhead) badhead->last = bad;
  badhead = bad;

  return bad;
}

aEX *add_ex(const char *mask, const char *from, const char *raison,
        time_t date, unsigned int flag)
{
  aEX *ex = calloc(1, sizeof *ex);

  if(!ex)
  {
    Debug(W_MAX|W_WARN, "add_ex: OOM for %s[%s] (%s)", mask, from, raison);
    return NULL;
  }

  str_dup(&ex->mask, mask);
  str_dup(&ex->raison, raison);
  Strncpy(ex->from, from, NICKLEN);
  ex->date = date;
  ex->flag = flag;
  ex->next = exhead;
  if(exhead) exhead->last = ex;
  exhead = ex;

  return ex;
}
