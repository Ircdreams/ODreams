/* src/operserv.c - Diverses commandes operserv pour admins
 * Copyright (C) 2021 bugsounet.fr
 *
 * contact: ircdreams@bugsounet.fr
 * site web: http://www.bugsounet.fr
 *
 * Services pour serveur IrcDreams V.3
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
 */

#include <ctype.h>
#include "main.h"
#include "debug.h"
#include "os_cmds.h"
#include "hash.h"
#include "outils.h"
#include "operserv.h"
#include "add_info.h"
#include "del_info.h"
#include "fichiers.h"
#include "aide.h"
#include "divers.h"
#include "admin_user.h"
#include "admin_manage.h"
#include "config.h"
#include "showcommands.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int first_register(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  anUser *user = NULL;
  aNick *n;
  aHashCmd *cmdp = FindCommand("REGISTER");

  if (!(n = getnickbynick(parv[1])))
    return osntc(nick, "%s n'est pas connecté.", parv[1]);

  if(!(n->flag & N_OPER))
    return osntc(nick, "Commande réservé aux Ircops !");

  if(!GetConf(CF_PREMIERE))
    return osntc(nick, "Commande réservé au premier lancement!");

  if((user = getuserinfo(parv[1])))
    return osntc(nick, "%s est déjà enregistré.", user->nick);

  user = add_regnick(parv[1], 10);

  osntc(nick, "Bienvenue sur les Services ODreams [" SPVERSION "] !");
  osntc(nick, "Vous êtes Administrateurs de niveau maximum.");

  ConfFlag &= ~CF_PREMIERE;
  cmdp->flag |= CMD_DISABLE;

  load_cmds();
  BuildCommandsTable(1);
  write_cmds();
  db_write_users();

  user = getuserinfo(parv[1]);
  nick->user = user;
  nick->user->n = nick;

  return 1;
}

int user(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  char *cmd = parv[1];
  anUser *user = NULL;
  int level = 0, i = 0;

  if(!strcasecmp(cmd, "ADD")) {
    if (parc < 3)
      return osntc(nick, "Syntaxe: USER ADD <username> <level>");
    if((user = getuserinfo(parv[2])))
      return osntc(nick, "%s est déjà enregistré.", user->nick);

    if(!is_num(parv[3]) || (level = atoi(parv[3])) > MAXADMLVL || level < 1)
      return osntc(nick, "Le level doit être compris entre 1 et %d", MAXADMLVL);

    if(level >= nick->user->level && nick->user->level != MAXADMLVL)
      return osntc(nick, "Vous ne pouvez pas donner un niveau supérieur au votre.");

    user = add_regnick(parv[2], atoi(parv[3]));
    db_write_users();

    osntc(nick, "%s a été enregistré au level %d", user->nick, user->level);
  }
  else if(!strcasecmp(cmd, "DEL")) {
    if (parc < 2)
      return osntc(nick, "Syntaxe: USER DEL <username>");
    if(!(user = getuserinfo(parv[2])))
      return osntc(nick, "%s n'est pas dans la base de donnée", parv[2]);
    if(level >= nick->user->level && nick->user->level != MAXADMLVL)
      return osntc(nick, "Vous ne pouvez pas donner un niveau supérieur au votre.");
    if(user->n)
      osntc(user->n, "Votre compte vient d'être supprimé par l'Administrateur %s", nick->nick);

    osntc(nick, "Le compte %s a bien été supprimé.", user->nick);
    del_regnick(user);
  }
  else if(!strcasecmp(cmd, "LIST")) {
    osntc(nick, "\2Niveau  Username\2");
    for(;i < USERHASHSIZE;++i) for(user = user_tab[i];user;user = user->next)
    osntc(nick, "%d       %-13s", user->level, user->nick);
  }
  else if(!strcasecmp(cmd, "LEVEL")) {
    if (parc < 3)
      return osntc(nick, "Syntaxe: USER LEVEL <username> <level>");
    if(!is_num(parv[3]) || (level = atoi(parv[3])) > MAXADMLVL || level < 0)
      return osntc(nick, "Veuillez préciser un level valide.");

    if(!(user = getuserinfo(parv[2])))
      return osntc(nick, "\2%s\2 n'est pas un UserName enregistré.", parv[2]);

    if(level >= nick->user->level && nick->user->level != MAXADMLVL)
      return osntc(nick, "Vous ne pouvez pas donner un niveau supérieur au votre.");

    if(level == 0)
      return osntc(nick, "Un Administrateur doit avoir un niveau > 0");
 
    if(level == user->level)
      return osntc(nick, "%s est déjà au niveau %d.", user->nick, level);
  
    if(level < ADMINLEVEL)
    {
      osntc(nick, "%s n'est plus Administrateur.", user->nick);
      if(user->n) adm_active_del(user->n);
    }
    else if(user->level < ADMINLEVEL)
    {
      osntc(nick, "%s est maintenant Administrateur au niveau\2 %d\2.", user->nick, level);
      if(user->n) adm_active_del(user->n);
    }
    else osntc(nick, "Vous avez modifié le niveau Administrateur de \2%s\2 en\2 %d\2.", user->nick, level);

    user->level = level;
    db_write_users();
  }
  else return osntc(nick, "Option inconnue: %s", cmd);
  return 1;
}

int xop(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  int i = 2;
  aNick *n;
  aJoin *join;
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);

  for (;i <= parc;i++)
  {
    if (!(n = getnickbynick(parv[i]))) {
      osntc(nick, "%s n'est pas connecté.", parv[i]);
      continue;
    }
    if(IsHiding(n)) {
      osntc(nick, "%s n'est pas connecté.", parv[i]);
      continue;
    }
    if(!(join = getjoininfo(n, parv[1]))) {
      osntc(nick, "%s n'est pas sur %s", n, parv[1]);
      continue;
    }
    if (IsOp(join)) {
      osntc(nick, "%s est déjà op sur %s", n, join);
      continue;
    }
    DoOp(join);
    putserv("%s " TOKEN_MODE " %s +o %s", bot.servnum, join, n->numeric);
    putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s +o %s par %s", os.num,
                  bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1], parv[i], num2servinfo(bot.servnum));

  }                               
  return 1;
}

int xhalf(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  int i = 2;
  aNick *n;
  aJoin *join;
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);
  
  for (;i <= parc;i++)
  {
    if (!(n = getnickbynick(parv[i]))) {
      osntc(nick, "%s n'est pas connecté.", parv[i]);
      continue;
    }
    if(IsHiding(n)) {
      osntc(nick, "%s n'est pas connecté.", parv[i]);
      continue;
    }
    if(!(join = getjoininfo(n, parv[1]))) {
      osntc(nick, "%s n'est pas sur %s", parv[i], parv[1]);
      continue;
    }
    if (IsHalf(join)) {
      osntc(nick, "%s est déjà halfop sur %s", n, join);
      continue;
    }
    DoHalf(join);
    putserv("%s " TOKEN_MODE " %s +h %s", bot.servnum, join, n->numeric);
    putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s +h %s par %s", os.num,
      bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1], parv[i], num2servinfo(bot.servnum));
   }
   return 1;
}

int xvoice(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  int i = 2;
  aNick *n;
  aJoin *join;
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);
  
  for (;i <= parc;i++)
  {
    if (!(n = getnickbynick(parv[i]))) {
      osntc(nick, "%s n'est pas connecté.", parv[i]);
      continue;
    }
    if(IsHiding(n)) {
      osntc(nick, "%s n'est pas connecté.", parv[i]);
      continue;
    }
    if(!(join = getjoininfo(n, parv[1]))) {
      osntc(nick, "%s n'est pas sur %s", n, parv[1]);
      continue;
    }
    if (IsVoice(join)) {
      osntc(nick, "%s est déjà voice sur %s", n, join);
      continue;
    }
    DoVoice(join);
    putserv("%s " TOKEN_MODE " %s +v %s", bot.servnum, join, n->numeric);
    putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s +v %s par %s", os.num,
      bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1], parv[2], num2servinfo(bot.servnum));

  }
  return 1;
}

int xdeop(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  int i = 2;
  aNick *n;
  aJoin *join;
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);

  for (;i <= parc;i++)
  {
    if (!(n = getnickbynick(parv[i]))) {
      osntc(nick, "%s n'est pas connecté.", parv[i]);
      continue;
    }
    if(IsHiding(n)) {
      osntc(nick, "%s n'est pas connecté.", parv[i]);
      continue;
    }
    if(!(join = getjoininfo(n, parv[1]))) {
      osntc(nick, "%s n'est pas sur %s", n, parv[1]);
      continue;
    }
    if (IsOp(join)) {
      DeOp(join);
      putserv("%s " TOKEN_MODE " %s -o %s", bot.servnum, join, n->numeric);
      putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s -o %s par %s", os.num,
        bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1], parv[2], num2servinfo(bot.servnum));
      continue;
    }
    osntc(nick, "%s est déjà deop sur %s", n, join);
  }
  return 1;
}

int xdehalf(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  int i = 2;
  aNick *n;
  aJoin *join;
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);
  
  for (;i <= parc;i++)
  {
     if (!(n = getnickbynick(parv[i]))) {
       osntc(nick, "%s n'est pas connecté.", parv[i]);
       continue;
     }
     if(!(join = getjoininfo(n, parv[1]))) {
       osntc(nick, "%s n'est pas sur %s", n, parv[1]);
       continue;
     }
     if (IsHalf(join)) {
       DeHalf(join);
       putserv("%s " TOKEN_MODE " %s -h %s", bot.servnum, join, n->numeric);
       putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s -h %s par %s", os.num,
         bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1], parv[2], num2servinfo(bot.servnum));
        continue;
     }
     osntc(nick, "%s est déjà dehalfop sur %s", n, join);
  }
  return 1;
}

int xdevoice(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  int i = 2;
  aNick *n;
  aJoin *join;
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);
        
  for (;i <= parc;i++)
  {
    if (!(n = getnickbynick(parv[i]))) {
      osntc(nick, "%s n'est pas connecté.", parv[i]);
      continue;
    }
    if(IsHiding(n)) {
      osntc(nick, "%s n'est pas connecté.", parv[i]);
      continue;
    }
    if(!(join = getjoininfo(n, parv[1]))) {
      osntc(nick, "%s n'est pas sur %s", n, parv[1]);
      continue;
    }
    if (IsVoice(join)) {
      DeVoice(join);
      putserv("%s " TOKEN_MODE " %s -v %s", bot.servnum, join, n->numeric);
      putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s -v %s par %s", os.num,
        bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1], parv[2], num2servinfo(bot.servnum));
        continue;
    }
    osntc(nick , "%s est déjà devoice sur %s", n, join);
  }
  return 1;
}

int xkick(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  aNick *n;
  aJoin *join;
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);
  char *msg = parv2msg(parc, parv, 3, 150);

  if(msg && *msg == ':') msg++;

  if (!(n = getnickbynick(parv[2]))) {
    osntc(nick, "%s n'est pas connecté.", parv[2]);
    return 1;
  }
  if(IsHiding(n)) {
    osntc(nick, "%s n'est pas connecté.", parv[2]);
    return 1;
  }
  if(!(join = getjoininfo(n, parv[1]))) {
    osntc(nick, "%s n'est pas sur %s", n, parv[1]);
    return 1;
  }
  putserv("%s " TOKEN_KICK " %s %s :%s", bot.servnum, join, n->numeric, (parc < 3) ? defraison : msg);
  putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037KICK\2\3 %s %s par %s (%s)", 
    os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1], parv[2], num2servinfo(bot.servnum), (parc < 3) ? defraison : msg);
  return 1;
}

int xkill(aNick *nick, aChan *chan, int parc, char **parv)
{
  char *msg = parv2msg(parc, parv, 2, 300);
  aNick *n;
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);

  if(msg && *msg == ':') msg++;

  if (!(n = getnickbynick(parv[1]))) {
    osntc(nick, "%s n'est pas connecté.", parv[1]);
    return 1;
  }
  if(n->flag & N_SERVICE) {
    osntc(nick, "\2%s\2 est un Service.", n->nick);
    return 1;
  }
  putserv("%s " TOKEN_KILL " %s :%s (%s) [%s]", os.num, n->numeric, os.nick, msg, nick->nick);
  putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] < \2\0037KILL\2\3 de %s par %s: %s [%s]", 
    os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, n->nick, os.nick, msg, nick->nick);
  del_nickinfo(n->numeric, "KILL");
  return 1;
}

int xblock(aNick *nick, aChan *chan, int parc, char **parv)
{
  aNick *n;
  char params[70];
  char *raison = parv2msg(parc, parv, 2, 300);  
  int end = CurrentTS + 3600;
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);
  
  if (!(n = getnickbynick(parv[1]))) {
    osntc(nick, "%s n'est pas connecté.", parv[1]);
    return 1;
  }
  if(n->flag & N_SERVICE) {
    osntc(nick, "\2%s\2 est un Service.", n->nick);
    return 1;
  }
  if(n->flag & N_OPER) {
    osntc(nick, "\2%s\2 est un Irc Opérateur.", n->nick);
    return 1;
  }

  if (parc < 2) strcpy(raison,blockmsg);

  strcat(raison, " -- ");
  strcat(raison, nick->user->nick);

  strcpy(params, "*@");
  strcat(params, GetIP(n->base64));

  putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] * \2\0037BLOCK\2\3 *!*@%s", 
    os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, GetIP(n->base64));
  putserv("%s GL * +%s 3600 %d %d :%s [Expire %s]", bot.servnum, params, tmt, end, raison, get_time(NULL,end));

  add_gline(params,CurrentTS + 3600, raison);
  write_gline();
  return 1;
}

int silence(aNick *nick, aChan *chan, int parc, char **parv)
{
  aNick *n;
  char params[70];
  char *raison = parv2msg(parc, parv, 2, 300);
  int end = CurrentTS + 300;
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);

  if (!(n = getnickbynick(parv[1]))) {
    osntc(nick, "%s n'est pas connecté.", parv[1]);
    return 1;
  }
  if(n->flag & N_SERVICE) {
    osntc(nick, "\2%s\2 est un Service.", n->nick);
    return 1;
  }
  if(n->flag & N_OPER) {
    osntc(nick, "\2%s\2 est un Irc Opérateur.", n->nick);
    return 1;
  }

  if (parc < 2) strcpy(raison, shunmsg);
  strcat(raison, " -- ");
  strcat(raison, nick->user->nick);

  strcpy(params, "*@");
  strcat(params, GetIP(n->base64));

  putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] * \2\0037SILENCE\2\3 *!*@%s", 
    os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, GetIP(n->base64));
  putserv("%s SU * +%s 300 :%s [Expire %s]", bot.servnum, params, raison, get_time(NULL,end));

  add_shun(params,CurrentTS + 300, raison);
  write_shun();
  return 1;
}

int xgline(aNick *nick, aChan *chan, int parc, char **parv)
{
  char *cmd = parv[1];
  char *host = parv[2];
  char *msg = parv2msg(parc, parv, 4, 300);
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);
  int tmpp = 0, count = 0;
  struct gline *tmp = glinehead, *save;

  if(msg && *msg == ':') msg++;

  if(!strcasecmp(cmd, "LIST")) {
    if(!glinehead) return osntc(nick, "La liste des Hosts-Glined est vide!");

    osntc(nick, "Liste des hosts glined :");
    for(;tmp;tmp = tmp->next) {
      osntc(nick, "Host: \002%s\2  Raison: %s [Expire %s]", tmp->host,tmp->raison, get_time(NULL,tmp->time));
      ++count;
    }
    osntc(nick, "Total: %d host%s glined", count, count > 1 ? "s" : "");
  }
  else if (!strcasecmp(cmd, "RDEF")) {
    if(!glinehead) return osntc(nick, "La liste des Hosts-Glined est vide!");

    for(;tmp;tmp = tmp->next) {
      if (tmp->time > CurrentTS)
        putserv("%s GL * +%s %d :%s [Expire %s]", bot.servnum, tmp->host, tmp->time - CurrentTS, tmp->raison, get_time(NULL,tmp->time));
    }
    osntc(nick, "Liste des hosts glined replacé avec succès!");
  }
  else if (!strcasecmp(cmd, "DEL")) {
    if (parc < 2)
      return osntc(nick, "Syntaxe: GLINE DEL <ident@host>");
    if(!glinehead)
      return osntc(nick, "La liste des Hosts-Glined est vide!");
    
    for(;tmp;tmp = save) {
      save = tmp->next;
      
      if(!mmatch(host,tmp->host)) {
        putserv("%s GL * -%s", bot.servnum, tmp->host);
        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] * \2\0037UNGLINE\2\3 %s",
          os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, tmp->host);
        osntc(nick, "Gline retiré: %s", tmp->host);
        del_gline(tmp->host);
        count++;
        write_gline();
      }
    }
    
    if(count) osntc(nick, "Total: %d effacé%s.", count, count > 1 ? "s" : "");
    else {
      osntc(nick, "Le host \2%s\2 n'est pas dans la liste", host);
      osntc(nick, "Utilisez l'argument FORCEDEL pour forcer à DEL");
      osntc(nick, "Syntaxe: GLINE FORCEDEL <ident@host>");
    }
    return 1;
  }
  else if(!strcasecmp(cmd, "FORCEDEL")) {
    if (parc < 2)
      return osntc(nick, "Syntaxe: GLINE FORCEDEL <ident@host>");
    putserv("%s GL * -%s %d %d", bot.servnum, host, CurrentTS, CurrentTS);
    putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] * \2\0037UNGLINE\2\3 %s", 
      os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, host); 
    osntc(nick, "Gline retiré: %s", host);
    return 1;
  }
  else if(!strcasecmp(cmd, "RAZ")) {
    if(!glinehead) return osntc(nick, "La liste des Hosts-Glined est vide!");

      for(;tmp;tmp = save) {
      save = tmp->next;
      putserv("%s GL * -%s", bot.servnum, tmp->host);
      putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] * \2\0037UNGLINE\2\3 %s",
        os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, tmp->host);
      osntc(nick, "Glined-Host supprimé: %s", tmp->host);
      del_gline(tmp->host);
      count++;
      write_gline();
    }
    osntc(nick, "Total: %d effacé%s.", count, count > 1 ? "s" : "");
  }
  else if (!strcasecmp(cmd, "ADD")) { 

    if (parc < 3) 
      return osntc(nick, "Syntaxe: GLINE ADD <ident@host> %<durée> [raison]");

    if(*host == '#') 
      return osntc(nick, "%s n'est pas un host correct.", host);

    if(match("*@*", host))
      return osntc(nick, "%s n'est pas un host correct.", host);

    if(!(*parv[3] == '%') || ((tmpp = convert_duration(++parv[3])) < 0))
      return osntc(nick, "Durée incorrecte, le format est %<XjXhXm> X étant le nombre respectif d'unités.");

    if (tmpp == 0)
      tmpp = 31536000;

    if (parc < 4) strcpy(msg,defraison);
    
    strcat(msg, " -- ");
    strcat(msg, nick->user->nick);
      
    putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] * \2\0037GLINE\2\3 %s -- Durée: %s -- Raison: %s",
      os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, host, duration(tmpp), msg);
    putserv("%s GL * +%s %d :%s  [Expire %s]", bot.servnum, host, tmpp, msg, get_time(NULL,CurrentTS + tmpp));  
    add_gline(host, CurrentTS + tmpp, msg);
    write_gline();
    osntc(nick, "Gline ajouté: %s -- Durée: %s -- Raison: %s", host, duration(tmpp), msg);
  }
  else return osntc(nick, "Option inconnue: %s", cmd);
  return 1;
}

int xshun(aNick *nick, aChan *chan, int parc, char **parv)
{
  char *cmd = parv[1];
  char *host = parv[2];
  char *msg = parv2msg(parc, parv, 4, 300);
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);
  int tmpp = 0, count = 0;
  struct shun *tmp = shunhead, *save;

  if(!strcasecmp(cmd, "LIST")) {
    if(!shunhead) return osntc(nick, "La liste des Hosts-Shun est vide!");

    osntc(nick, "Liste des hosts shun :");
    for(;tmp;tmp = tmp->next) {
      osntc(nick, "Host: \002%s\2  Raison: %s [Expire %s]", tmp->host,tmp->raison, get_time(NULL,tmp->time));
      ++count;
    }
    osntc(nick, "Total: %d host%s shun", count, count > 1 ? "s" : "");
  }
  else if (!strcasecmp(cmd, "RDEF")) {

    if(!shunhead) return osntc(nick, "La liste des Hosts-shun est vide!");

    for(;tmp;tmp = tmp->next)
    {
      if (tmp->time > CurrentTS)
        putserv("%s SU * +%s %d :%s [Expire %s]", bot.servnum, tmp->host, tmp->time - CurrentTS, tmp->raison, get_time(NULL,tmp->time));
    }
    osntc(nick, "Liste des hosts shun replacé avec succès!");
  }
  else if (!strcasecmp(cmd, "DEL")) {
    if (parc < 2)
      return osntc(nick, "Syntaxe: SHUN DEL <nick!ident@host>");
    if(!shunhead)
      return osntc(nick, "La liste des Hosts-shun est vide!");
    
    for(;tmp;tmp = save) {
      save = tmp->next;
      
      if(!mmatch(host,tmp->host)) {
        putserv("%s SU * -%s", bot.servnum, tmp->host);
        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] * \2\0037UNSHUN\2\3 %s",
          os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, tmp->host);
        osntc(nick, "Shun retiré: %s", tmp->host);
        del_shun(tmp->host);
        count++;
        write_shun();
      }
    }
    
    if(count) osntc(nick, "Total: %d effacé%s.", count, count > 1 ? "s" : "");
    else {
      osntc(nick, "Le host \2%s\2 n'est pas dans la liste", host);
      osntc(nick, "Utilisez l'argument FORCEDEL pour forcer à DEL");
      osntc(nick, "Syntaxe: SHUN FORCEDEL <nick!ident@host>");
    }
    return 1;
  }
  else if(!strcasecmp(cmd, "FORCEDEL")) {
    if (parc < 2)
      return osntc(nick, "Syntaxe: SHUN FORCEDEL <nick!ident@host>");
    putserv("%s SU * -%s", bot.servnum, host);
    putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] < \2\0037UNSHUN\2\3 %s",
      os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, host); 
    osntc(nick, "Shun retiré: %s", host);
    return 1;
  }
  else if(!strcasecmp(cmd, "RAZ")) {
    if(!shunhead) return osntc(nick, "La liste des Hosts-Shun est vide!");

    for(;tmp;tmp = save) {
      save = tmp->next;
      putserv("%s SU * -%s", bot.servnum, tmp->host);
      putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] * \2\0037UNSHUN\2\3 %s",
        os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, tmp->host);
      osntc(nick, "Shun-Host supprimé: %s", tmp->host);
      del_shun(tmp->host);
      count++;
      write_shun();
    }
    osntc(nick, "Total: %d effacé%s.", count, count > 1 ? "s" : "");
  }
  else if (!strcasecmp(cmd, "ADD")) { 

    if (parc < 3) 
      return osntc(nick, "Syntaxe: SHUN ADD <nick!ident@host> %<durée> [raison]");

    if(*host == '#') 
      return osntc(nick, "%s n'est pas un host correct.", host);

    if(match("*@*", host))
      return osntc(nick, "%s n'est pas un host correct.", host);

    if(!(*parv[3] == '%') || ((tmpp = convert_duration(++parv[3])) < 0))
      return osntc(nick, "Durée incorrecte, le format est %<XjXhXm> X étant le nombre respectif d'unités.");

    if (tmpp == 0)
      tmpp = 31536000;

    if (parc < 4) strcpy(msg,defraison);
    
    strcat(msg, " -- ");
    strcat(msg, nick->user->nick);
      
    putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] * \2\0037SHUN\2\3 %s -- Durée: %s -- Raison: %s",
      os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, host, duration(tmpp), msg);
    putserv("%s SU * +%s %d :%s  [Expire %s]", bot.servnum, host, tmpp, msg, get_time(NULL,CurrentTS + tmpp));  
    add_shun(host, CurrentTS + tmpp, msg);
    write_shun();
    osntc(nick, "Shun ajouté: %s -- Durée: %s -- Raison: %s", host, duration(tmpp), msg);
  }
  else return osntc(nick, "Option inconnue: %s", cmd);
  return 1;
}

int xsay(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  char *msg = parv2msg(parc, parv, 2, 400);

  if(msg && *msg == ':') msg++;
  
  if(*parv[1] != '#') {
    osntc(nick, "%s n'est pas un nom de salon.", parv[1]);
    return 1;
  }
  putserv("%s " TOKEN_PRIVMSG " %s :%s", bot.servnum, parv[1], msg);
  return 1;
}

int xdo(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  char *msg = parv2msg(parc, parv, 2, 400);

  if(msg && *msg == ':') msg++;

  if(*parv[1] != '#') {
    osntc(nick, "%s n'est pas un nom de salon.", parv[1]);
    return 1;
  }
  putserv("%s " TOKEN_PRIVMSG " %s :\1ACTION %s\1", bot.servnum, parv[1], msg);
  return 1;
}

int mjoin(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  int i=0;
  aNick *n;

  if(*parv[1] != '#') {
    osntc(nick, "%s n'est pas un nom de salon.", parv[1]);
    return 1;
  } 

  for(;i < NICKHASHSIZE;i++) 
  {
    for(n = nick_tab[i];n;n = n->next)
    {
      if(strcasecmp(n->nick, os.nick) && !IsService(n))
      {
        putserv("%s SJ %s %s", bot.servnum, n->numeric, parv[1]);
        osntc(n, "Un Administrateur vous force à rejoindre le salon %s", parv[1]);
      }
    }
  }
  return 1;
}

int mpart(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  int i=0;
  aNick *n;

  if(*parv[1] != '#') {
    osntc(nick, "%s n'est pas un nom de salon.", parv[1]);
    return 1;
  }

  for(;i < NICKHASHSIZE;i++)
  {
    for(n = nick_tab[i];n;n = n->next)
    {
      if(strcasecmp(n->nick, os.nick) && !IsService(n))
      {
        putserv("%s SP %s %s", bot.servnum, n->numeric, parv[1]);
        osntc(n, "Un Administrateur vous force à sortir du salon %s", parv[1]);
      }
    }
  }
  return 1;
}

int mgline(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  int i=0;
  aNick *n;
  aJoin *join;
  char *msg = parv2msg(parc, parv, 3, 300), params[70];
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);
  int tmp = 0;

  if(*parv[1] != '#')
    return osntc(nick, "%s n'est pas un nom de salon.", parv[1]);

  if(!(*parv[2] == '%') || ((tmp = convert_duration(++parv[2])) < 0))
    return osntc(nick, "Durée incorrecte, le format est %<XjXhXm> X étant le nombre respectif d'unités.");

  if (tmp == 0)
    tmp = 31536000;

  for(;i < NICKHASHSIZE;i++)
  {
    for(n = nick_tab[i];n;n = n->next)
    {
      if((join = getjoininfo(n, parv[1])) && !IsAnAdmin(n->user) && !IsOper(n))
      {
        if(parc < 3) strcpy(msg,defraison);
        if(msg && *msg == ':') msg++;

        strcat(msg, " -- ");
        strcat(msg, nick->user->nick);

        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037GLINE\2\3 *!*@%s -- Durée: %s -- Raison: %s",
          os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, GetIP(n->base64), duration(tmp), msg);
        putserv("%s GL * +*@%s %d :%s [Expire %s]", bot.servnum, GetIP(n->base64), tmp, msg, get_time(NULL,CurrentTS + tmp));
        strcpy(params, "*@");
        strcat(params, GetIP(n->base64));
        add_gline(params, CurrentTS + tmp, msg);
      }
    }
  }
  write_gline();
  return 1;
}

int mshun(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  int i=0;
  aNick *n;
  aJoin *join;
  char *msg = parv2msg(parc, parv, 3, 300), params[70];
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);
  int tmp = 0;

  if(*parv[1] != '#')
    return osntc(nick, "%s n'est pas un nom de salon.", parv[1]);

  if(!(*parv[2] == '%') || ((tmp = convert_duration(++parv[2])) < 0))
    return osntc(nick, "Durée incorrecte, le format est %<XjXhXm> X étant le nombre respectif d'unités.");

  if (tmp == 0)
    tmp = 31536000;

  for(;i < NICKHASHSIZE;i++)
  {
    for(n = nick_tab[i];n;n = n->next)
    {
      if((join = getjoininfo(n, parv[1])) && !IsAnAdmin(n->user) && !IsOper(n))
      {
        if(parc < 3) strcpy(msg,defraison);
        if(msg && *msg == ':') msg++;

        strcat(msg, " -- ");
        strcat(msg, nick->user->nick);

        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037SHUN\2\3 *!*@%s -- Durée: %s -- Raison: %s",
          os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, GetIP(n->base64), duration(tmp), msg);
        putserv("%s SU * +*@%s %d :%s [Expire %s]", bot.servnum, GetIP(n->base64), tmp, msg, get_time(NULL,CurrentTS + tmp));
        strcpy(params, "*@");
        strcat(params, GetIP(n->base64));
        add_shun(params, CurrentTS + tmp, msg);
      }
    }
  }
  write_shun();
  return 1;
}

int mkill(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  int i=0;
  aNick *n;
  aJoin *join;
  char *msg = parv2msg(parc, parv, 2, 300);
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);

  if(*parv[1] != '#') {
    osntc(nick, "%s n'est pas un nom de salon.", parv[1]);
    return 1;
  }
  if(msg && *msg == ':') msg++;

  for(;i < NICKHASHSIZE;i++)
  {
    for(n = nick_tab[i];n;n = n->next)
    {
      if((join = getjoininfo(n, parv[1])) && !IsAnAdmin(n->user) && !IsOper(n))
      {
        putserv("%s " TOKEN_KILL " %s :%s (%s) [%s]", os.num, n->numeric, os.nick, (parc < 2) ? defraison : msg, os.nick);
        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] < \2\0037KILL\2\3 de %s : %s [%s]",
          os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, n->nick, (parc < 2) ? defraison : msg, os.nick);
        del_nickinfo(n->numeric, "KILL");
      }
    }
  }
  return 1;
}

int mkick(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  int i=0;
  aNick *n;
  aJoin *join;
  char *msg = parv2msg(parc, parv, 2, 300);
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);

  if(*parv[1] != '#') {
    osntc(nick, "%s n'est pas un nom de salon.", parv[1]);
    return 1;
  }

  if(msg && *msg == ':') msg++;

  for(;i < NICKHASHSIZE;i++)
  {
    for(n = nick_tab[i];n;n = n->next)
    {
      if((join = getjoininfo(n, parv[1])) && !IsAnAdmin(n->user) && !IsOper(n))
      {
        putserv("%s " TOKEN_KICK " %s %s :%s [%s]", bot.servnum, join, n->numeric, (parc < 2) ? defraison : msg, os.nick);
        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037KICK\2\3 %s sur %s par %s -- Raison: %s [%s]",
          os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, n->nick, parv[1], os.nick, (parc < 2) ? defraison : msg, os.nick);
      }
    }
  }
  return 1;
}

int mop(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
        int i=0;
        aNick *n;
        aJoin *join;
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

  if(*parv[1] != '#') {
                 osntc(nick, "%s n'est pas un nom de salon.", parv[1]);
                 return 1;
        }

        for(;i < NICKHASHSIZE;i++)
        {
                for(n = nick_tab[i];n;n = n->next)
                {
                        if((join = getjoininfo(n, parv[1])) && !IsOp(join))
                        {
        putserv("%s " TOKEN_MODE " %s +o %s", bot.servnum, parv[1], n->numeric);
                                DoOp(join);
        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s +o %s par %s",
                                        os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                                        parv[1], n->nick, num2servinfo(bot.servnum));
                        }
                }
        }
        return 1;
}

int mdeop(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
        int i=0;
        aNick *n;
        aJoin *join;
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

  if(*parv[1] != '#') {
                 osntc(nick, "%s n'est pas un nom de salon.", parv[1]);
                 return 1;
        }

        for(;i < NICKHASHSIZE;i++)
        {
                for(n = nick_tab[i];n;n = n->next)
                {
                        if((join = getjoininfo(n, parv[1])) && IsOp(join))
                        {
                                putserv("%s " TOKEN_MODE " %s -o %s", bot.servnum, parv[1], n->numeric);
        DeOp(join);
                                putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s -o %s par %s",
                                        os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                                        parv[1], n->nick, num2servinfo(bot.servnum));
                        }
                }
        }
        return 1;
}

int mvoice(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
        int i=0;
        aNick *n;
        aJoin *join;
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

  if(*parv[1] != '#') {
                 osntc(nick, "%s n'est pas un nom de salon.", parv[1]);
                 return 1;
        }

        for(;i < NICKHASHSIZE;i++)
        {
                for(n = nick_tab[i];n;n = n->next)
                {
                        if((join = getjoininfo(n, parv[1])) && !IsVoice(join))
                        {
                                putserv("%s " TOKEN_MODE " %s +v %s", bot.servnum, parv[1], n->numeric);
        DoVoice(join);
                                putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s +v %s par %s",
                                        os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                                        parv[1], n->nick, num2servinfo(bot.servnum));
                        }
                }
        }
        return 1;
}

int mdevoice(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
        int i=0;
        aNick *n;
        aJoin *join;
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

  if(*parv[1] != '#') {
                 osntc(nick, "%s n'est pas un nom de salon.", parv[1]);
                 return 1;
        } 

        for(;i < NICKHASHSIZE;i++)
        {
                for(n = nick_tab[i];n;n = n->next)
                {
                        if((join = getjoininfo(n, parv[1])) && IsVoice(join))
                        {
                                putserv("%s " TOKEN_MODE " %s -v %s", bot.servnum, parv[1], n->numeric);
        DeVoice(join);
                                putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s -v %s par %s",
                                        os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                                        parv[1], n->nick, num2servinfo(bot.servnum));
                        }
                }
        }
        return 1;
}

int mhalf(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
        int i=0;
        aNick *n;
        aJoin *join;
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

  if(*parv[1] != '#') {
                 osntc(nick, "%s n'est pas un nom de salon.", parv[1]);
                 return 1;
        }

        for(;i < NICKHASHSIZE;i++)
        {
                for(n = nick_tab[i];n;n = n->next)
                {
                        if((join = getjoininfo(n, parv[1])) && !IsHalf(join))
                        {
                                putserv("%s " TOKEN_MODE " %s +h %s", bot.servnum, parv[1], n->numeric);
        DoHalf(join);
                                putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s +h %s par %s",
                                        os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                                        parv[1], n->nick, num2servinfo(bot.servnum));
                        }
                }
        }
        return 1;
}

int mdehalf(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
        int i=0;
        aNick *n;
        aJoin *join;
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

  if(*parv[1] != '#') {
                 osntc(nick, "%s n'est pas un nom de salon.", parv[1]);
                 return 1;
        }

        for(;i < NICKHASHSIZE;i++)
        {
                for(n = nick_tab[i];n;n = n->next)
                {
                        if((join = getjoininfo(n, parv[1])) && IsHalf(join))
                        {
                                putserv("%s " TOKEN_MODE " %s -h %s", bot.servnum, parv[1], n->numeric);
        DeHalf(join);
                                putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s -h %s par %s",
                                        os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                                        parv[1], n->nick, num2servinfo(bot.servnum));
                        }
                }
        }
        return 1;
}

int killhost(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
        int i=0;
        aNick *n;
        char *msg = parv2msg(parc, parv, 2, 300);
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

        for(;i < NICKHASHSIZE;i++)
        {
                for(n = nick_tab[i];n;n = n->next)
                {
                        if(!match(parv[1],n->host) && !IsAnAdmin(n->user) && !IsOper(n) && !(n->flag & N_SERVICE))
                        {
        putserv("%s " TOKEN_KILL " %s :%s (%s) [%s]", os.num, n->numeric, os.nick, (parc < 2) ? defraison : parv2msg(parc, 
        parv, 2, 150), nick->nick);
                                putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] * \2\0037KILLHOST\2\3 %s : %s [%s]",
                                        os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                                        n->nick, (parc < 2) ? defraison : msg, nick->nick);
        del_nickinfo(n->numeric, "KILL");
                        }
                }
        }
        return 1;
}

int closechan(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
        char *host = parv[1];
        char *msg = parv2msg(parc, parv, 3, 300);
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);
        int tmp = 0;

  if(msg && *msg == ':') msg++;

  if(*parv[1] != '#')
                 return osntc(nick, "%s n'est pas un nom de salon.", parv[1]);

  if(!(*parv[2] == '%') || ((tmp = convert_duration(++parv[2])) < 0))
    return osntc(nick, "Durée incorrecte, le format est %<XjXhXm> X étant le nombre respectif d'unités.");

  if (tmp == 0) 
                tmp = 31536000;

  if (parc < 3) strcpy(msg, defraison);
  strcat(msg, " -- ");
  strcat(msg, nick->user->nick);

        putserv("%s GL * +%s %d :%s", bot.servnum, host, tmp, msg);
        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] * \2\0037CLOSECHAN\2\3 %s -- Durée: %s -- Raison: %s",
                                        os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                                                host, duration(tmp), msg);
  add_gline(host, CurrentTS + tmp, msg);
        return 1;
}
int xban(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  char *mask = NULL;
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

        if(*parv[1] != '#') {
                 osntc(nick, "%s n'est pas un nom de salon.", parv[1]);
                 return 1;
        }

  if(!parv[2]) {
     osntc(nick, "Veuillez precisez un host.");
     return 1;
  }
  mask = pretty_mask(parv[2]);
  putserv("%s " TOKEN_MODE " %s +b %s", bot.servnum, parv[1], mask);
  putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s +b %s par %s",
                           os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                            parv[1], mask, os.nick);
        return 1;
}

int xunban(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

        if(*parv[1] != '#') {
                 osntc(nick, "%s n'est pas un nom de salon.", parv[1]);
                 return 1;
        }

        if(!parv[2]) {
                 osntc(nick, "Veuillez precisez un host.");
                 return 1;
        }
        putserv("%s " TOKEN_MODE " %s -b %s", bot.servnum, parv[1], parv[2]);
        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s -b %s par %s",
                           os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                            parv[1], parv[2], os.nick);
        return 1;
}

int xmode(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  int a = 0;
        char *modes = parv[2], *c = strchr(modes, 'o');
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

        if(*parv[1] != '#') return osntc(nick, "%s n'est pas un nom de salon.", parv[1]);

        if(c || (c = strchr(modes, 'b')) || (c = strchr(modes, 'v')) || (c = strchr(modes, 'h')))
                return osntc(nick, "Vous n'êtes pas autorisé à changer le mode '%c'", *c);

        if(strchr(modes, 'k')) a++;
        if(strchr(modes, 'l')) a++;

  putserv("%s " TOKEN_MODE " %s %s %s %s", bot.servnum, parv[1], modes, (a && parc > 2) ? 
    parv[3] : "", (a > 1 && parc > 3) ? parv[4] : "");
        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s %s %s %s par %s",
                    os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                      parv[1], modes, (a && parc > 2) ? parv[3] : "", (a > 1 && parc > 3) ? parv[4] : "", os.nick);
        return 1;
}

int check(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  char *host = parv[1];
  aNick *n;
  int i=0, clonage=0; 
  
  osntc(nick, "\2\0037Recherche sur\2\3 *@%s", host);

  for(;i < NICKHASHSIZE;i++)
        {
          for(n = nick_tab[i];n;n = n->next)
                {
      if(!match(host,n->host)) {
          osntc(nick, "\2\0037NICK\2\3 %s \2\0037ADRESSE\2\3 %s@%s"
                        , n->nick, n->ident, n->host);
          clonage++;
      }
    }
        }
  osntc(nick,"\2\0037TOTAL\2\3 %d", clonage);
  return 1;
}

int trust(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  char *ip = parv[2], *arg = parv[1];
  int count = 0;
  struct trusted *trust, *tmp = trusthead, *save;
  
        if(!strcasecmp(arg, "ADD")) {

    if(parc < 2)
                return osntc(nick, "Veuillez preciser une adresse ip.");

    if (!is_ip(ip) || strlen(ip) > 15)
    return osntc(nick, "L'adresse ip \2%s\2 n'est pas valide.", ip);

          if((trust = gettrusted(ip)))
                  return osntc(nick, "L'adresse ip \2%s\2 est déjà couverte.", ip);

    add_trusted(ip);
    append_file(TRUSTED_FILE, ip);
    write_trusted();
    return osntc(nick, "L'adresse ip \2%s\2 a bien été ajoutée", ip);
  }
  if(!strcasecmp(arg, "DEL")) {
  
    if(!trusthead) return osntc(nick, "La liste des Ips-Trusted est vide!");
    if(parc < 2) return osntc(nick, "Veuillez preciser une adresse ip.");

    for(;tmp;tmp = save) {
      save = tmp->next;
                        if(!match(parv[2],tmp->ip)) {
        osntc(nick, "Ip-Trusted retiré: %s", tmp->ip);
        del_trusted(tmp->ip);
        count++;
        write_trusted();
      }
    }
    if(count) osntc(nick, "Total: %d effacé%s.", count, count > 1 ? "s" : "");
    else osntc(nick, "L'Ips \2%s\2 n'est pas dans la liste", ip);
    return 1;
  }
  if(!strcasecmp(arg, "LIST")) {

          if(!trusthead) return osntc(nick, "La liste des Ips-Trusted est vide!");

          osntc(nick, "Liste des Ips-Ttrusted :");
          for(;tmp;tmp = tmp->next)
                  osntc(nick, "  \002%s\2", tmp->ip);
        return 1;
  }
  if(!strcasecmp(arg, "RAZ")) {

                if(!trusthead) return osntc(nick, "La liste des Ips-Trusted est vide!");

                for(;tmp;tmp = save) {
      save = tmp->next;
                        osntc(nick, "Ips-Trusted supprimé:  \002%s\2", tmp->ip);
      del_trusted(tmp->ip);
      count++;
    }
    osntc(nick, "Total: %d effacé%s.", count, count > 1 ? "s" : "");
    write_trusted();    
                return 1;
        }
  return syntax_cmd(nick, FindCoreCommand("trust"));
}

int stats(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  struct gline *gline = glinehead;
  struct trusted *trust = trusthead;
  struct shun *shun_ = shunhead;
  struct trace *trace_ = tracehead;

  int cmds = getoption("cmds", parv, parc, 1, 0);
  int traffic = getoption("traffic", parv, parc, 1, -1);
  int all = getoption("all", parv, parc, 1, -1);
  int serv = getoption("serv", parv, parc, 1, -1);
  int glined = getoption("gline", parv, parc, 1, -1);
  int trusted = getoption("trust", parv, parc, 1, -1);
  int shun = getoption("shun", parv, parc, 1, -1);
  int trace = getoption("trace", parv, parc, 1, -1);

  if(all) uptime(nick, chaninfo, parc, parv);
  
  if(cmds || all)
  {
    int x = 0, adm = 0, usr = 0;
    aHashCmd *cmd;

    for(;x < CMDHASHSIZE;x++) for(cmd = cmd_hash[x];cmd;cmd = cmd->next)
    {
      if(AdmCmd(cmd)) adm += cmd->used;
      else usr += cmd->used;
      if(cmds && *cmd->name != '\1' && !match(parv[cmds], cmd->name))
        osntc(nick, "- %s (\002%d\2)", cmd->name, cmd->used);
    }
    osntc(nick, "Un total de %d commandes utilisées, dont %d Admins (Ratio: %.2f%)",
      (adm + usr), adm, (adm / (float) (usr + adm)) * 100);
  }
  if(traffic || all)
  {
    double up, down;
    int mbu = 0, mbd = 0;

    up = bot.dataS/1024.0;
    down = bot.dataQ/1024.0;
    if(up >= 1024)
    {
      up /= 1024.0;
      mbu = 1;
    }
    if(down >= 1024)
    {
      down /= 1024.0;
      mbd = 1;
    }
    osntc(nick, "Traffic: Up:\2 %.3f %s\2 Down:\2 %.3f %s\2", up, mbu ? "MB":"KB", down, mbd ? "MB":"KB");
  }
  if(serv || all)
  {
    int i = 0, j=0, countuser = 0, countall = 0, countserv =0;

    for(;i < MAXNUM;i++)
    {
      if(serv_tab[i]) {
        countuser = 0;
        for(j = 0;j <= serv_tab[i]->maxusers;++j)
        {
          if(num_tab[i][j])
          {
            countuser++;
            countall++;
          }
        }
        osntc(nick, "Serveur: %s (%d %s)", serv_tab[i], countuser, countuser > 1 ? "Utilisateurs" : "Utilisateur");
        countserv++;
      }
    }
          osntc(nick, "Il y a %d %s sur %d %s", countall, countall > 1 ? "Utilisateurs" : "Utilisateur", countserv, countserv > 1 ? "Serveurs" : "Serveur" );
  }
  if(glined || all)
        {
    int countgline = 0;
          for(;gline;gline = gline->next) countgline++;
    osntc(nick, "Il y a %d Host Glined", countgline);
  }
  if(trace || all)
        {
                int counttrace = 0;
                for(;trace_;trace_ = trace_->next) counttrace++;
                osntc(nick, "Il y a %d Host Trace", counttrace);
        }
  if(shun || all)
        {
                int countshun = 0;
                for(;shun_;shun_ = shun_->next) countshun++;
                osntc(nick, "Il y a %d Host Shun", countshun);
        }
  if(trusted || all)
        {
    int counttrust = 0;
                for(;trust;trust = trust->next) counttrust++;
    osntc(nick, "Il y a %d Ip-Trusted", counttrust);
  }
  if(!all && !traffic && !cmds && !serv && !glined && !trusted && !shun) {
      osntc(nick, "Option inconnue.");
      osntc(nick, "Option disponible: ALL / CMDS <commande> / TRAFFIC / SERV / TRUST / GLINE / SHUN / TRACE");
  }
  return 1;
}

int trace(aNick *nick, aChan *chan, int parc, char **parv)
{
  char *cmd = parv[1], *mask = parv[2], *msg = parv2msg(parc, parv, 3, 300), hismask[NUHLEN + 1];
  time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);
  int count = 0, i = 0;
  struct trace *tmp = tracehead, *save;
  aNick *n;

  if(msg && *msg == ':') msg++;

  if(!strcasecmp(cmd, "LIST")) {
          if(!tracehead) return osntc(nick, "La liste de TRACE est vide!");

          osntc(nick, "Liste des hosts TRACE :");
          for(;tmp;tmp = tmp->next) {
                  osntc(nick, "MASK: \002%s\2  Raison: %s", tmp->mask,tmp->raison);
      ++count;
    }
    osntc(nick, "Total: %d mask%s", count, count > 1 ? "s" : "");
  }
  else if (!strcasecmp(cmd, "DEL")) {
    if (parc < 2)
      return osntc(nick, "Syntaxe: TRACE DEL <nick!ident@host>");
    if(!tracehead)
                        return osntc(nick, "La liste de TRACE est vide!");
    
    for(;tmp;tmp = save) {
      save = tmp->next;
      
                        if(!mmatch(mask,tmp->mask)) {
                    putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] ! \2\0037UNTRACE\2\3 %s",
                                                os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, tmp->mask);
                    osntc(nick, "TRACE retiré: %s", tmp->mask);
                    del_trace(tmp->mask);
        count++;
                    write_trace();
                        }
                }
    
    if(count) osntc(nick, "Total: %d effacé%s.", count, count > 1 ? "s" : "");
    else osntc(nick, "Le mask \2%s\2 n'est pas dans la liste", mask);

                return 1;
  }
  else if(!strcasecmp(cmd, "RAZ")) {
    if(!tracehead) return osntc(nick, "La liste de TRACE est vide!");

                for(;tmp;tmp = save) {
      save = tmp->next;
      putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] ! \2\0037UNTRACE\2\3 %s",
                                                os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, tmp->mask);
      osntc(nick, "TRACE supprimé: %s", tmp->mask);
      del_trace(tmp->mask);
      count++;
      write_trace();
    }
    osntc(nick, "Total: %d effacé%s.", count, count > 1 ? "s" : "");
  }
  else if (!strcasecmp(cmd, "ADD")) { 

    if (parc < 2) 
      return osntc(nick, "Syntaxe: TRACE ADD <nick!ident@host> [raison]");

    if(*mask == '#') 
                  return osntc(nick, "%s n'est pas un host correct.", mask);

    if(match("*!*@*", mask))
      return osntc(nick, "%s n'est pas un host correct.", mask);

    if (parc < 3) strcpy(msg,defraison);
    strcat(msg, " -- ");
          strcat(msg, nick->user->nick);
    
    putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] ! \2\0037TRACE\2\3 %s -- Raison: %s",
                                          os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                                                  mask, msg);
    add_trace(mask, msg);
    write_trace();
    osntc(nick, "TRACE ajouté: %s -- Raison: %s", mask, msg);
  }
  else if (!strcasecmp(cmd, "CHECK")) {
    if(!tracehead) return osntc(nick, "La liste de TRACE est vide!");

    for(;i < NICKHASHSIZE;i++)
                {
                  for(n = nick_tab[i];n;n = n->next)
                        {
                          strcpy(hismask, n->nick);
                                strcat(hismask, "!");
                                strcat(hismask, n->ident);
                                strcat(hismask, "@");
                                strcat(hismask, n->host);

                                for(;tmp;tmp = tmp->next) {
                                      if(!match(tmp->mask, hismask)) {
                                                osntc(nick,"TRACE: %s [Mask: %s] (Raison: %s)", hismask, tmp->mask, tmp->raison);
            ++count;
          }
        }
      }
    }
    osntc(nick, "Total: %d mask%s", count, count > 1 ? "s" : "");
  }
  else return osntc(nick, "Option inconnue: %s", cmd);
  return 1;
}

int ircop(aNick *nick, aChan *chan, int parc, char **parv)
{
  int i=0, count = 0;
  aNick *n;
  osntc(nick, "Liste des Ircops:");
  for(;i < NICKHASHSIZE;i++)
        {
          for(n = nick_tab[i];n;n = n->next)
                {
      if(IsOper(n) && !IsService(n)) {
        osntc(nick,"%s sur %s", n->nick, n->serveur);
        ++count;
      }
    }
  }
  osntc(nick, "Total: %d ircop%s", count, count > 1 ? "s" : "");
  return 1;
}

int helper(aNick *nick, aChan *chan, int parc, char **parv)
{
        int i=0, count = 0;
        aNick *n;
        osntc(nick, "Liste des Helpeurs:");
        for(;i < NICKHASHSIZE;i++)
        {
                for(n = nick_tab[i];n;n = n->next)
                {
                        if(IsHelper(n)) {
                                osntc(nick,"%s sur %s", n->nick, n->serveur);
                                ++count;
                        }
                }
        }
        osntc(nick, "Total: %d helpeur%s", count, count > 1 ? "s" : "");
        return 1;
}

int wall(aNick *nick, aChan *chan, int parc, char **parv)
{
  char *cmd = parv[1], *msg = parv2msg(parc, parv, 2, 300);
  int i=0;
        aNick *n;
  anUser *u;

  if(msg && *msg == ':') msg++;

  if (!strcasecmp(cmd, "OPER")) {
    for(;i < NICKHASHSIZE;i++)
                  for(n = nick_tab[i];n;n = n->next)
                          if(IsOper(n) && !IsService(n)) osntc(n,"[\2Wall Oper\2] %s - \002%s@%s\2", msg, nick->nick, nick->user);
  }
  else if (!strcasecmp(cmd, "HELPER")) {
          for(;i < NICKHASHSIZE;i++)
                  for(n = nick_tab[i];n;n = n->next)
                          if(IsHelper(n)) osntc(n,"[\2Wall Helper\2] %s - \002%s@%s\2", msg, nick->nick, nick->user);
  }
  
  else if (!strcasecmp(cmd, "USER")) putserv("%s "TOKEN_NOTICE" $*.* :[\2Notice Globale\2] %s - \002%s\2", os.num, msg, nick->nick);

  else if (!strcasecmp(cmd, "ADMIN")) {
    for(;i < USERHASHSIZE;++i) for(u = user_tab[i];u;u = u->next)
                  if(IsAdmin(u) && u->n) osntc(u->n,"[\2Wall Admin\2] %s - \002%s@%s\2", msg, nick->nick, nick->user);
  }
  else return osntc(nick, "Option inconnue: %s", cmd);
        return 1;
}
    
int svsjoin(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
  aNick *n;
  aJoin *join;
  time_t tmt = CurrentTS;
  struct tm *ntime = localtime(&tmt);

  if (!(n = getnickbynick(parv[2])))
    return osntc(nick, "%s n'est pas connecté.", parv[2]);
  if(IsHiding(n))
    return osntc(nick, "%s n'est pas connecté.", parv[2]);
  if(IsOper(n))
                return osntc(nick, "%s est ircop.", parv[2]);
  if((join = getjoininfo(n, parv[1])))
    return osntc(nick, "%s est déjà sur %s", parv[2], parv[1]);
  if(*parv[1] != '#')
    return osntc(nick, "%s n'est pas un nom de salon valide.", parv[1]);

  putserv("%s SJ %s %s", bot.servnum, n->numeric, parv[1]);
  putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] * \2\0037SVSJOIN\2\3 %s %s par %s", os.num,
                bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1], parv[2], os.nick);
  osntc(n, "Un administrateur vous force à rejoindre le salon %s.", parv[1]);
  return 1;
}

int svspart(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
        aNick *n;
        aJoin *join;
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

        if(!(n = getnickbynick(parv[2])))
                return osntc(nick, "%s n'est pas connecté.", parv[2]);
  if(IsHiding(n))
    return osntc(nick, "%s n'est pas connecté.", parv[2]);
        if(IsOper(n))
                return osntc(nick, "%s est ircop.", parv[2]);
        if(!(join = getjoininfo(n, parv[1])))
                return osntc(nick, "%s n'est pas sur %s", parv[2], parv[1]);
  if(*parv[1] != '#')
                return osntc(nick, "%s n'est pas un nom de salon valide.", parv[1]);

        putserv("%s SP %s %s", bot.servnum, n->numeric, parv[1]);
        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] * \2\0037SVSPART\2\3 %s %s par %s", os.num,
                bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1], parv[2], os.nick);
  osntc(n, "Un administrateur vous force à sortir du salon %s.", parv[1]);

        return 1;
}

int svsnick(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
        aNick *n;
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

        if (!(n = getnickbynick(parv[1])))
                return osntc(nick, "%s n'est pas connecté.", parv[1]);
  if(IsHiding(n))
    return osntc(nick, "%s n'est pas connecté.", parv[1]);
        if(IsOper(n))
                return osntc(nick, "%s est ircop.", parv[1]);

  putserv("%s "TOKEN_SVSNICK" %s :%s", bot.servnum, n->numeric, parv[2]);
        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] * \2\0037SVSNICK\2\3 %s %s par %s", os.num,
                bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1], parv[2], os.nick);
  osntc(n, "Un administrateur a changé votre pseudonyme en %s.", parv[2]);

        return 1;
}

int svshost(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
        aNick *n;
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

        if (!(n = getnickbynick(parv[1])))
                return osntc(nick, "%s n'est pas connecté.", parv[1]);
  if(IsHiding(n))
    return osntc(nick, "%s n'est pas connecté.", parv[1]);
        if(IsOper(n))
                return osntc(nick, "%s est ircop.", parv[1]);

  putserv("%s SVSHOST %s %s", bot.servnum, n->numeric, parv[2]);
        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] * \2\0037SVSHOST\2\3 %s %s par %s", os.num,
                bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1], parv[2], os.nick);
  osntc(n, "Un administrateur a changé votre hostname en %s.", parv[2]);

        return 1;
}
