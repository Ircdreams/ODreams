/* src/serveur.c - Traitement des messages IRC
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
 * $Id: serveur.c,v 1.27 2005/12/07 17:16:07 bugs Exp $
 */

#include "main.h"
#include "outils.h"
#include "serveur.h"
#include "add_info.h"
#include "admin_manage.h"
#include "hash.h"
#include "os_cmds.h"
#include "config.h"
#include "del_info.h"
#include "debug.h"
#include "aide.h"
#include <sys/time.h>
#include "fichiers.h"
#include "operserv.h"
#include <netinet/in.h>
#include <arpa/inet.h>

aServer *mainhub = NULL;
static int burst = 0;


int m_account(int parc, char **parv)
{
	aNick *nick = NULL;
	anUser *user;

	if(parc < 3 || !(nick = num2nickinfo(parv[1])))
	{
		if ((nick = num2nickinfo(parv[1]))) {
                	nick->user = NULL;
		}
		else 
			Debug(W_DESYNCH|W_PROTO|W_WARN, "Erreur dans un TOKEN AC de %s: arg=%d nick=%s",
                                        parv[0], parc, "NULL");
                return 0;

	}

	if((user = getuserinfo(parv[2])))
	{
		nick->user = user;
		nick->user->n = nick;
		if(nick->user->level >0)
			osntc(nick, "\2Vous êtes identifié en tant qu'Administrateur niveau %d\2", nick->user->level);

	/*
	 * dans la série je code pour rien ...
	 * j'ai essayé de forcé le flag oper au login ... résultat fake OPER !
	 * juste les umodes WCIO peuvent etre appliqué
	 * nous ne pouvons pas kill !
	 * level 10 = admin
	 * level 6 a 9 = coadmin
	 * level 1 a 5 = ircop
	 * si qqn a envie de codé qqch de mieux ...
	 * je laisse le code
	 * Merci de me tenir au courant quand meme !
	 *
	 *	if(nick->user->level == 10)
	 *		putserv("%s " TOKEN_SVSMODE" %s +ogswa", bot.servnum, nick->numeric);
	 *	if(nick->user->level >= 6 && nick->user->level <10)
	 *		putserv("%s " TOKEN_SVSMODE" %s +ogswc", bot.servnum, nick->numeric);
	 *	if(nick->user->level >= 1 && nick->user->level <6)
	 *		putserv("%s " TOKEN_SVSMODE" %s +ogsw", bot.servnum, nick->numeric);
	 *
	 */
	
	}

	return 0;
}


/*
 * m_clearmode SSCCC CM # mode_cleared
 */
int m_clearmode(int parc, char **parv)
{
	int clear = 0;
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);
        register aNick *nick;
        aJoin *join;
        int i = 0;


        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037CLEARMODE\2\3 %s par %s (%s)", os.num, bot.pchan, ntime->tm_hour,
        ntime->tm_min, ntime->tm_sec, parv[1], GetInfobyPrefix(parv[0]), parv[2]);

	if(strchr(parv[2], 'o')) clear |= J_OP;
	if(strchr(parv[2], 'h')) clear |= J_HALF;
	if(strchr(parv[2], 'v')) clear |= J_VOICE;

	if(!clear) return 0; /* aucun changement de +o/+v*/
	for(;i < NICKHASHSIZE;++i)
		for(nick = nick_tab[i];nick;nick = nick->next)
			if(!(nick->flag & (N_SERVICE|N_GOD)) && (join = getjoininfo(nick, parv[1])))
				join->status &= ~clear;
	return 0;
}

/*
 * m_mode SSCCC M #|SSCCC +|-m
 */
int m_mode(int parc, char **parv)
{
	time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);
	aNick *nick = GetInfobyPrefix(parv[0]), *nptr = NULL;

	if(parc < 3) return Debug(W_PROTO|W_WARN, "#arg invalide: MODE de %s: #arg=%d", parv[0], parc);
	if(*parv[1] == '#')
	{
		aChan *ch = getchaninfo(parv[1]);
		char *modes = parv[2], *param = NULL, *key = NULL, *limit = NULL, params[500];
		aJoin *join = NULL;
		int what = 1, i = 2, obv = 1;

		params[0]='\0';

		while (*modes)
		{
			switch (*modes)
			{
				case '+':
					what = 1;
					break;
				case '-':
					what = 0;
					break;
				case 'o':
					param = parv[++i];
					if(!(nptr = num2nickinfo(param)))
					{
						Debug(W_DESYNCH|W_WARN, "m_mode: aNick non trouvé pour '%s %co %s'", parv[1], what ? '+' : '-', param);
						break;
					}
					strcat(params, nptr->nick);
					strcat(params, " ");

					if((join = ch ? GetJoinIbyC(nptr, ch) : getjoininfo(nptr, parv[1])))
					{
						if(what) DoOp(join);
						else DeOp(join);
					}
					break;
				
				case 'h': /* mise à jour du status */
					if((nptr = num2nickinfo(parv[++i])) && (join = ch ? GetJoinIbyC(nptr, ch) : getjoininfo(nptr, parv[1])))
					{
						if(what) DoHalf(join);
						else DeHalf(join);
					}
					strcat(params, nptr->nick);
					strcat(params, " ");
					break;

				case 'v': /* mise à jour du status */
					if((nptr = num2nickinfo(parv[++i])) && (join = ch ? GetJoinIbyC(nptr, ch) : getjoininfo(nptr, parv[1])))
					{
						if(what) DoVoice(join);
						else DeVoice(join);
					}
					strcat(params, nptr->nick);
					strcat(params, " ");
					break;

				case 'b':
					param = parv[++i]; /* grab parameter (mask) */
					strcat(params, param);
					strcat(params, " ");
					break;

				default: /* other modes than OBV! (Need Parse!) */
					if(*modes == 'l' && what) { limit = parv[++i]; strcat(params, parv[i]); strcat(params, " "); }
					else if(*modes == 'k') { key = parv[++i]; strcat(params, parv[i]); strcat(params, " "); }
					obv = 0;
			} /* switch */
			modes++;
		} /* while */

		if(GetInfobyPrefix(parv[0]))
		putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s %s %spar %s", os.num,
		bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1], parv[2], params, 
		GetInfobyPrefix(parv[0]));
		else
		putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s %s %spar %s", os.num,
                bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1], parv[2], params,
                num2servinfo(parv[0]));
		
	} /* if(ch) */
	else if(nick)
	{
	nick->flag = parse_umode(nick->flag, parv[2]);
	
	if(GetInfobyPrefix(parv[0]))
	putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s %s par %s", os.num,
        bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1], parv[2], GetInfobyPrefix(parv[0]));
	else
	putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037MODE\2\3 %s %s par %s", os.num,
        bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1], parv[2], num2servinfo(parv[0]));
		
	if(*parv[2] == '+')
	{
		char *modes = parv[2];
		for(modes = parv[2]; *modes; modes++)
		{
                        if(*modes == 'a') {
			  putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] ! \2\0037OPER\2\3 %s est un Irc Administrateur (+a)", os.num, bot.pchan,
                          ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1]);
                          putserv("%s " TOKEN_WALLOPS " :%s est un Irc Administrateur", os.num,parv[1]);
 			}
			if(*modes == 'c') {
                          putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] ! \2\0037OPER\2\3 %s est un Irc Co-Administrateur (+c)", os.num, bot.pchan,
                          ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1]);
                          putserv("%s " TOKEN_WALLOPS " :%s est un Irc Co-Administrateur", os.num,parv[1]);
                        }
			if(*modes == 'o' && !(nick->flag & N_ADM) && !(nick->flag & N_COADM))
			{
		 	  putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] ! \2\0037OPER\2\3 %s est un Irc Opérateur (+o)", os.num, bot.pchan, 
			  ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1]);
			  putserv("%s " TOKEN_WALLOPS " :%s est un Irc Opérateur", os.num,parv[1]);
			}
		}
	}

	if(*parv[2] == '-')
        {
        	char *modes = parv[2];
                for(modes = parv[2]; *modes; modes++)
                {
			if(*modes == 'a') {
                          putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] ! \2\0037OPER\2\3 %s n'est plus un Irc Administrateur (-a)", os.num,
				bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1]);
                        }
			if(*modes == 'c') {
                          putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] ! \2\0037OPER\2\3 %s n'est plus un Irc Co-Administrateur (-c)", os.num,
                                bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1]);
                        }
		        if(*modes == 'o' && !(nick->flag & N_ADM) && !(nick->flag & N_COADM))
                        {
 			  putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] ! \2\0037OPER\2\3 %s n'est un plus Irc Opérateur (-o)", os.num, bot.pchan,
				ntime->tm_hour, ntime->tm_min, ntime->tm_sec, parv[1]);
                        }                                          
		}          		                          
	}

	}
	
	
	return 0;
}

/*
 * m_part ABAAA L # [:raison]
 */
int m_part(int parc, char **parv)
{
	time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);
	char *p, *ptr = NULL;
	aNick *nick = NULL;

	if(parc < 2) return Debug(W_PROTO|W_WARN, "#arg invalide: PART de %s: #arg=%d", parv[0], parc);

	if(!(nick = GetInfobyPrefix(parv[0])))
		return Debug(W_DESYNCH|W_WARN, "PART %s de %s: aNick non trouvé!", parv[1], parv[0]);

	putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] < \2\0037PART\2\3 %s par %s :%s", os.num, bot.pchan, ntime->tm_hour,
		ntime->tm_min, ntime->tm_sec, parv[1], nick, parc > 2 ? parv[2] : "Aucune raison");

	for(p = Strtok(&ptr, parv[1], ',');p;p = Strtok(&ptr, NULL, ',')) del_join(nick, p);
	return 0;
}

/*
 * m_kick SSCCC K # SSCCC :raison
 */
int m_kick(int parc, char **parv)
{
	time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);
	aNick *v;
	
	if(GetInfobyPrefix(parv[0]))
	putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] > \2\0037KICK\2\3 %s sur %s par %s (%s)", os.num, bot.pchan, ntime->tm_hour,
                ntime->tm_min, ntime->tm_sec, GetInfobyPrefix(parv[2]), parv[1], GetInfobyPrefix(parv[0]), parv[3]);
	else
	putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] > \2\0037KICK\2\3 %s sur %s par %s (%s)", os.num, bot.pchan, ntime->tm_hour,
                ntime->tm_min, ntime->tm_sec, GetInfobyPrefix(parv[2]), parv[1], num2servinfo(parv[0]), parv[3]);

	if(parc < 3) return Debug(W_PROTO|W_WARN, "#arg invalide: KICK de %s: #arg=%d", parv[0], parc);

	if(!(v = num2nickinfo(parv[2])))
		return Debug(W_DESYNCH|W_WARN, "KICK de %s sur %s par %s: cible introuvable?!", parv[0], parv[2], parv[1]);

	del_join(v, parv[1]);

	return 0;
}

/*
 * m_join SSCCC J # TS
 */
int m_join(int parc, char **parv)
{
	time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);
	aNick *nick = NULL;
	aBAD *bad;	

	if(parc < 2) return Debug(W_PROTO|W_WARN, "#arg invalide: JOIN de %s: arg=%d", parv[0], parc);

	if(!(nick = GetInfobyPrefix(parv[0])))
		return Debug(W_DESYNCH|W_WARN, "JOIN %s de %s: aNick non trouvé!", parv[1], parv[0]);

	putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] > \2\0037JOIN\2\3 %s par %s", os.num, bot.pchan, ntime->tm_hour, 
		ntime->tm_min, ntime->tm_sec, parv[1], nick);

	if(*parv[1] == '0') /* partall !!*/
	{
		aJoin *join = nick->joinhead, *jt;
		for(;join;join = jt)
		{
			jt = join->next;
			del_join(nick, join->channel);
		}
	}

	else add_join(nick, parv[1], 0, CurrentTS, getchaninfo(parv[1]));

	/* arrivé sur un badchan */
        if (IsBadChan(parv[1]) && !IsExChan(parv[1])) {
		bad = IsBadChan(parv[1]);
        	putserv("%s GL * +%s 3600 :%s [%s]", bot.servnum, parv[1], bad->raison, os.nick);
        }

	return 0;
}

/*
 * m_create SSCCC C # TS
 */
int m_create(int parc, char **parv)
{
	time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);
	char *p, *ptr = NULL;
	aNick *nick;
	aBAD *bad;

	if(parc < 3) return Debug(W_PROTO|W_WARN, "#arg invalide: CREATE de %s: arg=%d", parv[0], parc);
	if(!(nick = num2nickinfo(parv[0])))
		return Debug(W_DESYNCH|W_WARN, "CREATE %s de %s: aNick non trouvé!", parv[1], parv[0]);

	putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] > \2\0037CREATE\2\3 %s par %s", os.num, bot.pchan, ntime->tm_hour, 
		ntime->tm_min, ntime->tm_sec, parv[1], nick);

	for(p = Strtok(&ptr, parv[1], ',');p;p = Strtok(&ptr, NULL, ','))
		add_join(nick, p, J_OP, atol(parv[2]), getchaninfo(p));

	/* arrivé sur un badchan */
        if (IsBadChan(parv[1]) && !IsExChan(parv[1])) {
		bad = IsBadChan(parv[1]);		
        	putserv("%s GL * +%s 3600 :%s [%s]", bot.servnum, parv[1], bad->raison, os.nick);
        }
	
	return 0;
}

/*
 *m_nick SS N nick (hop) TS ident host.com [+mode [args ...]] IPBASE64 num realname
 */
int m_nick(int parc, char **parv)
{
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);
        char *nick = parv[1], *ac = NULL, params[70], buf[NICKLEN + 1], hismask[NUHLEN + 1];
        int i=0, clonage=0, end = CurrentTS + 3600;
        aNick *who = NULL, *n;
        anUser *user;
	aBAD *bad;
	struct trace *trace = tracehead;

        if(parc > 7) /*Nouveau nick*/
        {
                aServer *serv = num2servinfo(parv[0]);

                who = add_nickinfo(nick, parv[4], parv[5], parv[parc-3], parv[parc-2],
                        /* aServ*,      real-name ,     timestamp       ,       umodes */
                        serv, parv[parc-1], atol(parv[3]), *parv[6] == '+' ? parv[6] : NULL);

                if(!who) return 0; /* sauvetage temporaire..*/

                if(!burst)
                        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] > \2\0037PSEUDO\2\3 %s \2\0037ADRESSE\2\3 %s@%s"
                                " \2\0037SERVEUR\2\3 %s", os.num, bot.pchan, ntime->tm_hour, ntime->tm_min,
                                ntime->tm_sec, nick, parv[4], parv[5], serv);

                for(;i < NICKHASHSIZE;i++)
                        {
                                for(n = nick_tab[i];n;n = n->next)
                                {
					if(tracehead && !burst) { // on ne sais jamais ...
						strcpy(hismask, who->nick);
                                        	strcat(hismask, "!");
                                        	strcat(hismask, who->ident);
                                        	strcat(hismask, "@");
                                        	strcat(hismask, who->host);
						for(;trace;trace = trace->next) {
							if(!match(trace->mask, hismask)) {
								oswallops("TRACE: %s se connecte [Mask: %s] (Raison: %s)", hismask, 
									trace->mask, trace->raison);
								putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] > \2\0034TRACE\2\3 %s \2\0034MASK\2\3 %s"
			                         	        	" \2\0034RAISON\2\3 %s", os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, 
									ntime->tm_sec, hismask, trace->mask, trace->raison);
							}
						}
					}

                                        if(!strcasecmp(n->host,who->host) && (strcasecmp(who->host,os.host)))
                                                        clonage++;
                                }
                        }


                if((clonage > 1) && (!gettrusted(GetIP(who->base64))) && !burst)
                        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] > \2\0037CLONES\2\3 *@%s \2\0037NOMBRE\2\3 %d/%d"
                		,os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, who->host, clonage, clonemax);

                if((clonage > 1) && (gettrusted(GetIP(who->base64))) && !burst)
                        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] > \2\0037CLONES\2\3 *@%s \2\0037NOMBRE\2\3 %d \2\0034(Trusted)\2\3"
                		,os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, who->host, clonage);

                if((clonage > (clonemax -2)) && (!gettrusted(GetIP(who->base64))) && !burst) {
                        int i=0;
			oswallops("CLONES *@%s NOMBRE %d/%d", who->host, clonage, clonemax);
                        for(;i < NICKHASHSIZE;i++)
                        {
                                for(n = nick_tab[i];n;n = n->next)
                                {
                                        if(!strcasecmp(who->host,n->host))
                                                osntc(n, "Votre adresse de connexion *@%s est arrivée au nombre maximum de clones autorisés. "
                                                        "Merci de bien vouloir en supprimer.", n->host);
                                }
                        }
                }

		if((clonage > (clonemax -1)) && (!gettrusted(GetIP(who->base64)))) {
                        strcpy(params, "*@");
                        strcat(params, GetIP(who->base64));

                        putserv("%s GL * +%s 3600 :Trop de Clones [Expire %s]", bot.servnum, params, get_time(NULL,end));
                        add_gline(params,end, "Trop de clones");
                        oswallops("CLONES *@%s G-Lined [Trop de Clones]", who->host, clonage, clonemax);
                }

                if(*parv[6] == '+')
                {
                        int i = 7;
                        if(who->flag & N_REG) ac = parv[i++];
                }

                if(ac)
                {
			if(!(user = getuserinfo(ac)) || user->n) return 0;
                        who->user = user;
                        who->user->n = who;
                        if(IsAdmin(user)) adm_active_add(who);
                }

        }
        else if(parc <= 3) /* Changement de pseudo */
        {
                if(!(who = num2nickinfo(parv[0])))
                        return Debug(W_DESYNCH|W_WARN, "NICK %s > %s: aNick non trouvé!", parv[0], nick);

                putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037NICK\2\3 de %s en %s", os.num, bot.pchan,
                                                ntime->tm_hour, ntime->tm_min, ntime->tm_sec, who, nick);
                switch_nick(who, nick);
        }
	else return Debug(W_PROTO|W_WARN, "#arg invalide: NICK %s %s %s: arg=%d", parv[0], parv[1], parv[2], parc);
	
        if(IsBadNick(nick) && !IsExNick(nick) && !IsOper(who) && (!who->user || !IsAdmin(who->user)))
        {
                                bad = IsBadNick(nick);
                                osntc(who, "\2Avertissement\2 : Vous venez de prendre un pseudo illégal");
                                osntc(who, "Raison: %s", bad->raison);
                                do {
                                        sprintf(buf, "%.20s%d", chnick, rand() & 1023);
                                } while(getnickbynick(buf));
                                putserv("%s "TOKEN_SVSNICK" %s :%s", bot.servnum, who->numeric, buf);
                                return 0;
        }
	return 0;
}

/*
 * m_burst SS B # TS [mode key lim] users %ban
 */

int m_burst(int parc, char **parv)
{
	aNick *nick = NULL;
	aChan *c = NULL;
	char *modes = NULL, *chan = parv[1], *users = NULL;
	char *flags = NULL, *num = NULL, *key = NULL, *limit = NULL;
	int status = 0, i = 4;
	long il = atol(parv[2]);

	if(parc < 4)
		return parc < 3 ? Debug(W_PROTO|W_WARN, "#arg invalide: BURST de %s: arg=%d", parv[0], parc) : 0;
		/* le parc == 3 n'est pas hors protocole (même si ça parait bizarre et que de toute façon on s'en sert pas)
		 * > s'il la liste d'user est vide c'est qu'il y a des zombies */
	if(*parv[3] == '+')
	{  /* channel has mode(s) */
		modes = parv[3];
		if(strchr(modes, 'l')) limit = parv[i++];
		if(strchr(modes, 'k')) key = parv[i++];
		users = parv[i];
	}
	else if(*parv[3] != '%') users = parv[3];
	else return 0; /*bans only*/

	if(!strcasecmp(bot.pchan, chan)) /* salon de log */
		num2nickinfo(os.num)->floodtime = il; /* small hack to clean up */
	
	for(num = Strtok(&key, users, ',');num;num = Strtok(&key, NULL, ','))
	{
		if(*(flags = &num[2*NUMSERV+1]) == ':') /* SSCCC[:[o|v]] recherche directe sans while*/
		{
			status = 0;
			*flags++ = '\0'; /* efface le ':' pour n'avoir que la num*/
			if(*flags == 'o') status = J_OP, flags++;/* recherche directe sans while*/
			if(*flags == 'h') status |= J_HALF, flags++;
			if(*flags == 'v') status |= J_VOICE, flags++;
		}
		if((nick = num2nickinfo(num))) add_join(nick, chan, status | J_BURST, il, c);
		else Debug(W_DESYNCH|W_WARN, "BURST %s sur %s de %s: aNick non trouvé!", num, parv[1], parv[0]);
	}
	return 0;
}

/*
 * m_quit SSCCC Q :raison
 */
int m_quit(int parc, char **parv)
{
	time_t tmt = CurrentTS;
	struct tm *ntime = localtime(&tmt);
	putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] < \2\0037QUIT\2\3 de %s : %s", os.num, bot.pchan, ntime->tm_hour,
                ntime->tm_min, ntime->tm_sec, GetInfobyPrefix(parv[0]), parv[1]);
	del_nickinfo(parv[0], "QUIT");
	return 0;	
}

/*
 * m_kill SSCCC D SSCC :path (raison)
 */
int m_kill(int parc, char **parv)
{
	time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);
	aNick *nick;
	char *raison = strchr(parv[2], ' ');

	if(parc < 3) return Debug(W_PROTO|W_WARN, "#arg invalide: KILL de %s: arg=%d", parv[0], parc);
	/* Ghost 5 (Echo envoyé par le serveur de la victime lorsque Z envoie un KILL) */
	if(!(nick = num2nickinfo(parv[1])) && parv[2][strlen(os.nick)] == ' '
		&& !strncmp(parv[2], os.nick, strlen(os.nick))) return 0;
	else if(!nick) return Debug(W_DESYNCH|W_WARN, "KILL de %s sur %s: aNick non trouvé!", parv[0], parv[1]);

	if(GetInfobyPrefix(parv[0]))
	putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] < \2\0037KILL\2\3 de %s par %s :%s", os.num, bot.pchan, ntime->tm_hour,
                ntime->tm_min, ntime->tm_sec, nick, GetInfobyPrefix(parv[0]), raison);
	else
	putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] < \2\0037KILL\2\3 de %s par %s :%s", os.num, bot.pchan, ntime->tm_hour,
                ntime->tm_min, ntime->tm_sec, nick, num2servinfo(parv[0]), raison);

	del_nickinfo(parv[1], "KILL");
	return 0;
}

/*
 * m_away SSCCC A [msg]
 */
int m_away(int parc, char **parv)
{
    aNick *nick = GetInfobyPrefix(parv[0]);

	if(!nick) return Debug(W_DESYNCH|W_WARN, "AWAY de %s: aNick non trouvé!", parv[0]);
	if(parc > 1 && *parv[1]) nick->flag |= N_AWAY;
	else
	{
		nick->flag &= ~N_AWAY;
	}
	return 0;
}

/*
 * m_whois
 */
int m_whois(int parc, char **parv)
{
	aNick *n;
	int remote = 0;

	if(!strcasecmp(parv[2], os.nick)) n = num2nickinfo(os.num);
	else if(!strcasecmp(parv[2], os.nick)) n = num2nickinfo(os.num);

	else n = getnickbynick(parv[2]), remote = 1;

	if(!n) return 0;

	putserv("%s 311 %s %s %s %s * :%s", bot.servnum, parv[0], n->nick, n->ident,
		(n->user && n->flag & N_CRYPT) ? "hidden" : n->host, n->name);

	putserv("%s 312 %s %s %s :%s", bot.servnum, parv[0], parv[2],
		n->serveur->serv, remote ? n->name : bot.name);

	if(IsOper(n)) putserv("%s 313 %s %s :est un IRC Opérateur %s", bot.servnum, parv[0],
		parv[2], remote ?  "tyrannique et pervers" : "- Service");

	if(!remote) putserv("%s 317 %s %s %ld %ld :Secondes d'idle, secondes d'uptime",
		 bot.servnum, parv[0], parv[2], CurrentTS - bot.uptime, bot.uptime);

	putserv("%s 318 %s %s :Fin de la commande /WHOIS", bot.servnum, parv[0], parv[2]);
	return 0;
}

int m_topic(int parc, char **parv)
{
        time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

        if(parc < 2) return Debug(W_PROTO|W_WARN, "#arg invalide: TOPIC de %s: arg=%d", parv[0], parc);

	if(!burst)
	{
	        if(num2servinfo(parv[0]))
        	        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037TOPIC\2\3 %s %s \3par %s", os.num,
                                bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                                parv[1], parv[parc-1], num2servinfo(parv[0]));
	        else
        	        putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0037TOPIC\2\3 %s %s \3par %s", os.num,
                                bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                                parv[1], parv[parc-1], num2nickinfo(parv[0]));
	}
        return 0;
}

/*
 * m_eob SS EB SS
 */
int m_eob(int parc, char **parv)
{
	time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);
	aServer *serv = num2servinfo(parv[0]);

	if (!serv || !(serv->flag & ST_BURST))
		return Debug(W_DESYNCH|W_WARN, "EOB de %s: serveur inconnu/déjà synchro?", parv[0]);

	serv->flag &= ~ST_BURST;
	serv->flag |= ST_ONLINE;

	if(serv == mainhub) /* mon uplink a fini son burst, j'envoie la fin du mien!*/
	{
		putserv("%s "TOKEN_BURST" %s %ld +Omintrs %s:o", bot.servnum, bot.pchan, num2nickinfo(os.num)->floodtime, os.num);
		putserv("%s " TOKEN_EOB, bot.servnum);
		putserv("%s " TOKEN_EOBACK, bot.servnum);

		if(bot.dataQ) {
			struct timeval tv;
			double tt;
			gettimeofday(&tv, NULL);/*un peu de stats sur le burst pour *fun* */
			tt = (tv.tv_usec - burst) / 1000000.0;
			oswallops("Connexion de ODreams %s effectuée", SPVERSION);
			putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] = \2\0033BURST\2\3 Connexion de ODreams %s effectuée", 
				os.num, bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec, SPVERSION);
			if(!deconnexion) 
				load_gline();
				load_shun();
			}

		burst = 0;
	}
	return 0;
}

int m_server(int parc, char **parv)
{
	if(parc < 7) return Debug(W_PROTO|W_WARN, "#arg invalide: SERVER de %s: arg=%d", parv[0], parc);

	return add_server(parv[1], parv[6], parv[2], parv[5], parv[0]);
}

static int do_squit(int servnum)
{
	int i = 0;

	for(;i < MAXNUM;++i)
		if(serv_tab[i] && serv_tab[i]->hub == serv_tab[servnum])/* i est un leaf de servnum */
			do_squit(i);/* squit récursif de ses leaf */

	for(i = 0;i <= serv_tab[servnum]->maxusers;++i)
	{
		if(num_tab[servnum][i]) /* purge des clients de 'i' */
		{
			del_nickinfo(num_tab[servnum][i]->numeric, "SQUIT");
			num_tab[servnum][i] = NULL;
		}
	}

	free(num_tab[servnum]);
	num_tab[servnum] = NULL;
	free(serv_tab[servnum]);
	serv_tab[servnum] = NULL;
	return 1;
}

int m_squit(int parc, char **parv)
{
	int i = 0;
	time_t tmt = CurrentTS;
	struct tm *ntime = localtime(&tmt);

	putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] < \2\0037SQUIT\2\3 de %s", os.num, bot.pchan, ntime->tm_hour,
                ntime->tm_min, ntime->tm_sec, parv[1]);

	if(parc < 2) return Debug(W_PROTO|W_WARN, "#arg invalide: SQUIT de %s: arg=%d", parv[0], parc);

	for(;i < MAXNUM;++i)
		if(serv_tab[i] && !strcasecmp(serv_tab[i]->serv, parv[1]))
			return do_squit(i);

	return Debug(W_DESYNCH|W_WARN, "Squit d'un serveur inconnu %s!", parv[1]);
}

int m_pass(int parc, char **parv)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	burst = tv.tv_usec; /* TS en µs pour les stats */

	putlog(LOG_PARSES, "Connecté au serveur %s", bot.ip);
	bot.lasttime = tv.tv_sec;
	bot.lastbytes = bot.dataQ;
	return 0;
}

int m_ping(int parc, char **parv)
{
	struct timeval tv;
	char *theirttm;

	if(parc < 4) 
        { 
                putserv("%s " TOKEN_PONG " %s", bot.servnum, parv[1]); 
                return 0; 
        } 
        if((theirttm = strchr(parv[3], '.'))) theirttm++; 
	gettimeofday(&tv, NULL);
/* AsLL */
	putserv("%s " TOKEN_PONG " %s %s %s %ld %ld.%ld", bot.servnum, bot.servnum,
			parv[0], parv[3], ((tv.tv_sec - atoi(parv[3])) * 1000 + (tv.tv_usec - atoi(theirttm)) / 1000),
			tv.tv_sec, tv.tv_usec);/* mon TS seconde.micro*/
    return 0;
}

int m_error(int parc, char **parv)
{
	return Debug(W_MAX|W_TTY, "Erreur reçue du Hub (%s)", parv[1]);
}

static int exec_cmd(aHashCmd *cmd, unsigned int secure, aNick *nick, int parc, char **parv)
{
	char *tmp3 = NULL, buff[512];
	aChan *chaninfo = NULL;
	anAccess *acces = NULL;

	if(SecureCmd(cmd) && !secure) osntc(nick, "La commande \2%s\2 demande des informations privées. Veuillez utiliser \2/%s %s %s\2.d",
			cmd->name, os.nick, cmd->name, parv2msg(parc, parv, 1, 400));

	cmd->used++;
	if(cmd->flag & CMD_DISABLE)
		return osntc(nick, "Commande désactivée.");

	if(parc < cmd->args || (!parc && ChanCmd(cmd)))	return syntax_cmd(nick, cmd);

	if(cmd->level)
	{
		if(!nick->user || !nick->user->n)
			return osntc(nick, "%s", pasdeperm);

		if(AdmCmd(cmd) && nick->user->level < cmd->level)
			return osntc(nick, "Vous devez être Administrateur des Services pour utiliser cette commande.");

		if(ChanCmd(cmd) && !IsAdmin(nick->user))
		{
			if(!(acces = GetAccessIbyUserI(nick->user, chaninfo)))
				return osntc(nick, "Vous n'avez pas accès à %s.", parv[1]);

			if(cmd->level > acces->level)
				return osntc(nick, "Votre niveau sur %s est trop bas pour utiliser cette commande (Vous: \2%d\2, Commande: \2%d\2)",
					parv[1], acces->level, cmd->level);
		}
	}

	if(((ChanCmd(cmd)) || !Secure2Cmd(cmd)) && parc)
		tmp3 = parv2msg(parc, parv, 1, 400);
	
	if(nick->user) fastfmt(buff, "\00312\2$\2\3 $ par $@$", cmd->name, tmp3, nick->nick, nick->user->nick);
	else fastfmt(buff, "\00312\2$\2\3 $ par $", cmd->name, tmp3, nick->nick);
	
	putchan(buff);

	putlog(LOG_CMDS, "%s", buff);
	cmd->func(nick, chaninfo, parc, parv);/* passage du aChan * en plus, pour économie */
	return 1;
}

/*
 * m_privmsg SSCCC P SSCCC|# :msg
 * pparv[0] = nom de la commande
 * pparv[pparc] = arguments de la commande
 * ATTENTION: pparc est le nb d'arg à partir de *parv[1]*
 */
int m_privmsg(int parc, char **parv)
{
	char *tmp, *pparv[250], *ptr = NULL;
	unsigned int pparc = 0, securise = 0;
	aHashCmd *cmd;
	aNick *nptr = GetInfobyPrefix(parv[0]);

	if(!nptr) return 0;/* la source du message n'est pas un user connu (serveur/desynch)*/

	if(*parv[1] != '#') /* Message privé */
	{
                pparv[1] = NULL;
		if((tmp = strchr(parv[1], '@'))) /* P nick@server */
		{
			*tmp = 0;
			securise = 1;
		}
		
		pparv[0] = (tmp = Strtok(&ptr, parv[2], ' '));
		if(!tmp) return 0;

		if(!(cmd = FindCommand(tmp)))
		{
			if(*tmp != '\1') osntc(nptr, "La commande \2%s\2 n'existe pas.", tmp);
			return 0;
		}

		while((tmp = Strtok(&ptr, NULL, ' '))) pparv[++pparc] = tmp;

		exec_cmd(cmd, securise, nptr, pparc, pparv);
	}
	return 0;
}
