/* src/outils.c - Divers outils
 * Copyright (C) 2005 ircdreams.org
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
 * $Id: outils.c,v 1.12 2005/06/23 16:39:30 bugs Exp $
 */

#include <ctype.h>

#include <fcntl.h>
#include <unistd.h> 

#include "main.h"
#include "debug.h"
#include "os_cmds.h"
#include "admin_manage.h"
#include "hash.h"
#include "outils.h"
#include <netinet/in.h>
#include <arpa/inet.h>

int split_buf(char *buf, char **parv, int size)
{
        int parc = 0, i;

        while(*buf && parc < size)
        {
                while(*buf == ' ') *buf++ = 0; /* on supprime les espaces pour le découpage */
                if(*buf == ':') /* last param */
                {
                        parv[parc++] = buf + 1;
                        break;
                }
                if(!*buf) break;
                parv[parc++] = buf;
                while(*buf && *buf != ' ') ++buf; /* on laisse passer l'arg.. */
        }
        for(i = parc;i < size;++i) parv[i] = NULL; /* NULLify following items */
        return parc;
}

static const struct Modes {
	char mode;
	int value;
} UMode[] = {
	{ 'i', N_INV },
	{ 'x', N_CRYPT },
	{ 'r', N_REG },
	{ 'o', N_OPER },
	{ 'k', N_SERVICE },
	{ 'a', N_ADM },
	{ 'Z', N_GOD },
	{ 'f', N_FEMME },
	{ 'h', N_HOMME },
	{ 'g', N_DEBUG },
	{ 'w', N_WALLOPS },
	{ 'D', N_DIE },
	{ 'd', N_DEAF },
	{ 'H', N_SPOOF },
	{ 'A', N_HELPER },
	{ 'W', N_WHOIS },
	{ 'C', N_CHANNEL },
	{ 'I', N_IDLE },
	{ 'P', N_PRIVATE },
	{ 'X', N_HIDE },
	{ 'c', N_COADM },
},
	CMode[] = {
	{ 'n', C_MMSG },
	{ 't', C_MTOPIC },
	{ 'i', C_MINV },
	{ 'l', C_MLIMIT },
	{ 'k', C_MKEY },
	{ 's', C_MSECRET },
	{ 'p', C_MPRIVATE },
	{ 'm', C_MMODERATE },
	{ 'c', C_MNOCTRL },
	{ 'C', C_MNOCTCP },
	{ 'O', C_MOPERONLY },
	{ 'r', C_MUSERONLY },
	{ 'A', C_MHELPER },
	{ 'R', C_MACCONLY },
	{ 'N', C_MNONOTICE },
	{ 'q', C_MNOQUITPARTS },
	{ 'z', C_MSSLONLY },
	{ 'D', C_MAUDITORIUM },
};

int is_num(const char *num)
{
   while(*num) if(!isdigit(*num++)) return 0;
   return 1;
}

int is_ip(const char *ip)
{
	int dot = 0, digits = 0;
	char d[4] = {0};

	for(;*ip;ip++) {
		if(*ip != '.') {
			if(!isdigit(*ip) || digits == 3) return 0;
			d[digits++] = *ip;
		}
		else {
		    d[digits] = 0;
			if(!digits || ++dot > 3 || atoi(d) > 255) return 0;
			digits = 0;
		}
	}

	d[digits] = 0;
	return (dot == 3 && digits && atoi(d) < 256);
}

void strlwr(char *buf)
{
	for(;*buf;++buf) *buf = tolower(*buf);
}

void strip_newline(char *string)
{
   register char *p = string;
   while(*p && *p != '\n' && *p != '\r') ++p;
   *p = '\0';
}

char *GetIP(const char *base64)
{
	struct in_addr addr2;

	addr2.s_addr = htonl(base64toint(base64));
	return inet_ntoa(addr2);
}

struct trusted *gettrusted(const char *ip)
{
  struct trusted *tmp = trusthead;
  for(;tmp && match(tmp->ip, ip);tmp = tmp->next);
  return tmp;
}

struct gline *getgline(const char *host)
{
  struct gline *tmp = glinehead;
  for(;tmp && match(tmp->host, host);tmp = tmp->next);
  return tmp;
}

struct trace *gettrace(const char *host)
{
  struct trace *tmp = tracehead;
  for(;tmp && match(tmp->mask, host);tmp = tmp->next);
  return tmp;
}

struct shun *getshun(const char *host)
{
  struct shun *tmp = shunhead;
  for(;tmp && match(tmp->host, host);tmp = tmp->next);
  return tmp;
}

/*
 * getoption:
 * recherche dans un tableau char ** (de 'parc' entrées)
 * l'expression "opt" à partir de l'élément 'deb'
 * -si exp_int est > 0:
 *  cela veut dire qu'on attend une valeur numerique apres le char 'opt'
 *  si c'est le cas, la fct renvoit la valeur, sinon 0
 * -si exp_int est < 0:
 *  cela veut dire que l'on recherche uniquement si 'opt' est dans parv,
 *  si c'est le cas, retourne 1
 * -si exp_int est = 0:
 *  cela veut dire que l'on attend une string apres le 'opt',
 *  si c'est le cas retourne l'index de cette string dans parv, sinon 0.
 *
 */

int getoption(const char *opt, char **parv, int parc, int deb, int exp_int)
{
	for(;deb <= parc;++deb)
		if(!strcasecmp(opt, parv[deb]))
		{
			if(exp_int < 0) return 1;
			if(++deb <= parc)
			{
				if(!exp_int) return deb;
				else if(is_num(parv[deb])) return atoi(parv[deb]);
			}
			break;
		}
	return 0;
}

char *parv2msg(int parc, char **parv, int base, int size)
{
	int i = base, toksize, written = 0;
	static char buf[512];

	if(size > sizeof buf) size = sizeof buf;

	for(;i <= parc;++i)
	{
		toksize = strlen(parv[i]);

		if(i > base) buf[written++] = ' ';

		if(written + toksize >= size-1)
		{
			Strncpy(buf + written, parv[i], size - written - 1);
			written = size - 1;
			break;
		}
		else
		{
			strcpy(buf + written, parv[i]);
			written += toksize;
		}
	}
	buf[written] = 0;
	return buf;
}

char *duration(int s)
{
	static char dur[200];
	int i = 1;
	dur[0] = '\002'; 
    
        if(s >= 86400) 
                i += sprintf(dur + i, "%d", s/86400), s %= 86400, strcpy(dur + i, "\2 jours \002"), i += 9;
        if(s >= 3600) 
                i += sprintf(dur + i, "%d", s/3600), s %= 3600, strcpy(dur + i, "\2 heures \002"), i += 10; 
        if(s >= 60) 
                i += sprintf(dur + i, "%d", s/60), s %= 60, strcpy(dur + i, "\2 minutes \002"), i += 11; 
        if(s) i += sprintf(dur + i, "%d",s), strcpy(dur + i, "\2 secondes"); 
        else dur[i-2]= 0; 

	return dur;
}

time_t convert_duration(const char *p)/* extrait une durée en secondes*/
{										/* d'un format XdXhXm.. - Cesar*/
	const char *p1 = p;
	time_t result = 0;

	while(*p1)
	{
		if(!isdigit(*p1) && *p1 != 'h' && *p1 != 'H' && *p1 != 'j'
			&& *p1 != 'J' && *p1 != 'd' && *p1 != 'D' && *p1 != 'Y'
			&& *p1 != 'y' && *p1 != 'm')
		{
			result = -1;
			break;
		}
		switch(*p1)
		{
			case 'H': case 'h':
						result += 3600 * strtol(p, NULL, 10);
						p = p1 + 1;
						break;
			case 'm':
						result += 60 * strtol(p, NULL, 10);
						p = p1 + 1;
						break;
			case 'Y': case 'y':
						result += 31536000 * strtol(p, NULL, 10);
						p = p1 + 1;
						break;
			case 'd': case 'j':
			case 'J': case 'D':
						result += 86400 * strtol(p, NULL, 10);
						p = p1 + 1;
						break;
		}
		p1++;
	}
	if(result >= 0) result += strtol(p, NULL, 10);
	return result;
}

char *get_time(aNick *nick, time_t mytime)
{
	static char buftime[TIMELEN + 1];
	register struct tm *lt = localtime(&mytime);
	int i = 0;
	const char *ptr = {0};

	if(lt->tm_wday == 0) ptr = "Dimanche";
	if(lt->tm_wday == 1) ptr = "Lundi";
	if(lt->tm_wday == 2) ptr = "Mardi";
	if(lt->tm_wday == 3) ptr = "Mercredi";
	if(lt->tm_wday == 4) ptr = "Jeudi";
	if(lt->tm_wday == 5) ptr = "Vendredi";
	if(lt->tm_wday == 6) ptr = "Samedi";

        while((buftime[i++] = *ptr++)); 
    
        buftime[i-1] = ' '; 
        buftime[i] = ' '; 
	
	snprintf(buftime + i + 1, sizeof buftime,"%02d-%02d-%d %02d:%02d:%02d",
		lt->tm_mday, lt->tm_mon + 1, 1900 + lt->tm_year, lt->tm_hour, lt->tm_min, lt->tm_sec);
	return buftime;	
}

char *GetNUHbyNick(aNick *nick, int type)
{
	static char nuhbuf[NUHLEN];
	register char *ptr = nick->nick, *ptr2 = nuhbuf;

	while((*ptr2++ = *ptr++));
	*(ptr2-1) = '!';
	for(ptr = nick->ident;(*ptr2++ = *ptr++););
	*(ptr2-1) = '@';
	for(
                ptr = nick->host
	;(*ptr2++ = *ptr++);
	
	);

	return nuhbuf;
}

char *GetPrefix(aNick *nick)
{
	static char pref[12];
	int i = 0;
	pref[0] = '\0';

	if(IsOper(nick)) strcpy(pref, "\2\0037*\2\003"), i = 6;
	if(nick->user)
	{
		if(IsAdmin(nick->user)) strcpy(pref + i, "\00312^\003");
		else strcpy(pref + i, "\0035!\003");
	}
	return pref;
}

void putlog(const char *fichier, const char *fmt, ...)
{
	va_list vl;
	FILE *fp = fopen(fichier, "a");

	if(!fp) return;

	fputc('[', fp);
	fputs(get_time(NULL, CurrentTS), fp);
	fputs("] ", fp);

	va_start(vl, fmt);
	vfprintf(fp, fmt, vl);
	va_end(vl);

	fputc('\n', fp);
	fclose(fp);
}

int parse_umode(int flag, const char *p)
{/* flag de départ*/
	unsigned int w = 1, i;

	if(!p) return flag;
	for(;*p;p++)
	{
		if(*p == '+') w = 1;
		else if(*p == '-') w = 0;
		else
			for (i = 0;i < ASIZE(UMode);i++)
				if(*p == UMode[i].mode)
				{
					if(w) flag |= UMode[i].value;
					else flag &= ~UMode[i].value;
					break;
				}
	}
	return flag;
}

char *GetModes(int flag)
{
	static char mode[ASIZE(UMode)];
	unsigned int i = 0, j = 0;
	for(;i < ASIZE(UMode);i++) if(flag & UMode[i].value) mode[j++] = UMode[i].mode;
	mode[j] = '\0';
	return mode;
}

int cmodetoflag(int flag, const char *mode)
{
	unsigned int w = 1, i;

	if(!mode) return flag;
	for(;*mode;mode++)
	{
		if(*mode == '+') w = 1;
		else if(*mode == '-') w = 0;
		else for (i = 0;i < ASIZE(CMode);i++)
				if(*mode == CMode[i].mode)
				{
					if(w) flag |= CMode[i].value;
					else flag &= ~CMode[i].value;
					break;
				}
	}
	return flag;
}

int IsValidNick(char *p)
{
	if(*p == '-' || isdigit(*p)) return 0;
	while(*p)
	{
		if(!isalnum(*p) && !strchr(SPECIAL_CHAR, *p)) return 0;
		p++;
	}
	return 1;
}

char *RealCmd(const char *cmd)
{       /* returns actual name of a command, given its core name */
	aHashCmd *cmdp = FindCoreCommand(cmd);
	return cmdp ? cmdp->name : "";
}

char *str_dup(char **to, const char *from)
{
	if(from && *from)
	{
		*to = realloc(*to, strlen(from) + 1);
		if(!*to) Debug(W_MAX, "EXIT: strdup: no memory for 'to'(%p)=%s 'from'(%p)=%s\n",
			(void *) *to, *to, (const void *) from, from);
		else strcpy(*to, from);
	}
	return *to;
}

char *Strncpy(char *to, const char *from, size_t n)/* copie from dans to. ne copie que n char*/
{							/* MAIS AJOUTE LE CHAR \0 à la fin, from DOIT donc faire n+1 chars.*/
	const char *end = to + n;
	char *save = to;
	while(to < end && (*to++ = *from++));
	*to = 0;
	return save;
}

char *Strtok(char **save, char *str, int sep)/* fonction tirée d'ircu, simplifiée */
{
	register char *pos = *save; 	/* keep last position across calls */
	char *tmp;

	if(str) pos = str; 	/* new string scan */

	while(pos && *pos && sep == *pos) pos++; /* skip leading separators */

	if (!pos || !*pos) return (pos = *save = NULL); /* string contains only sep's */

	tmp = pos; /* now, keep position of the token */

	if((pos = strchr(pos, sep))) *pos++ = 0;/* get sep -Cesar*/

	*save = pos;
	return tmp;
}

inline int fastfmtv(char *buf, const char *fmt, va_list vl) 
{ 
        const char *s = buf; 
        while(*fmt) 
        { 
                if(*fmt == '$') 
                { 
                        register char *tmp = va_arg(vl, char *); 
                        if(tmp) while(*tmp) *buf++ = *tmp++; 
                } 
                else *buf++ = *fmt; 
                ++fmt; 
        } 
        *buf = 0; 
        return buf - s; 
} 
    
int fastfmt(char *buf, const char *fmt, ...) 
{ 
        va_list vl; 
        int i; 
    
        va_start(vl, fmt); 
        i = fastfmtv(buf, fmt, vl); 
        va_end(vl); 
        return i; 
} 

/* Repris en majorité d'ircu (Carlo Wood (Run)) */
char *pretty_mask(char *mask)
{
	static char star[2] = "*", retmask[NUHLEN];
	char *last_dot = NULL, *ptr = mask;
	/* Case 1: default */
	char *nick = mask, *user = star, *host = star;

	/* Do a _single_ pass through the characters of the mask: */
	for(; *ptr; ++ptr)
	{
		if(*ptr == '!')
		{	/* Case 3 or 5: Found first '!' (without finding a '@' yet) */
			user = ++ptr;
			host = star;
		}
		else if(*ptr == '@')
		{	/* Case 4: Found last '@' (without finding a '!' yet) */
			nick = star;
			user = mask;
			host = ++ptr;
		}
		else if(*ptr == '.')
		{	/* Case 2: Found last '.' (without finding a '!' or '@' yet) */
			last_dot = ptr;
			continue;
		}
		else continue;

		for(; *ptr; ++ptr) if(*ptr == '@') host = ptr + 1;
		break;
	}
	if(user == star && last_dot)
	{	/* Case 2: */
		nick = star;
		host = mask;
	}
	/* Check lengths */
	if(nick != star)
	{
		char *nick_end = (user != star) ? user - 1 : ptr;
		if(nick_end - nick > NICKLEN) nick[NICKLEN] = 0;
		*nick_end = 0;
	}
	if(user != star)
	{
		char *user_end = (host != star) ? host - 1 : ptr;
		if(user_end - user > USERLEN)
		{
			user = user_end - USERLEN;
			*user = '*';
		}
		*user_end = 0;
	}
	if(host != star && ptr - host > HOSTLEN)
	{
		host = ptr - HOSTLEN;
		*host = '*';
	}

        if(!*host) host = star; /* prevent '*!@*' or 'a!b@' */
        if(!*user) user = star;
        if(!*nick) nick = star; /* ... and !b@c */

	for(ptr = retmask;*nick;++nick) if(*nick != '*' || ptr == retmask || nick[-1] != '*') *ptr++ = *nick;
	for(*ptr++ = '!';*user;++user) if(*user != '*' || ptr[-1] != '*') *ptr++ = *user;
	for(*ptr++ = '@';*host;++host) if(*host != '*' || ptr[-1] != '*') *ptr++ = *host;
	*ptr = 0;
	return retmask;
}
