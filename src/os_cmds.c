/* src/cs_cmds.c commandes IRC du CS
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
 * $Id: os_cmds.c,v 1.2 2005/05/22 15:58:51 bugs Exp $
 */

#include "main.h"
#include "os_cmds.h"
#include "admin_manage.h"
#include "outils.h"
#include "hash.h"
#include <sys/socket.h>

void putserv(const char *fmt, ...)
{
	char buf[512];
	int len;
	va_list vl;

	va_start(vl, fmt);
	len = vsnprintf(buf, sizeof buf - 2, fmt, vl);
	va_end(vl);

	if(len < 0) len = sizeof buf - 2;

#ifdef DEBUG
	if(strncmp(buf+NUMSERV+1, TOKEN_PONG, 9-4*NUMSERV) && strncmp(buf+NUMSERV+1, TOKEN_PING, 9-4*NUMSERV))
    putlog(LOG_PARSES, "S - %s", buf);
#endif

	buf[len++] = '\r';
	buf[len++] = '\n';
	send(bot.sock, buf, len, 0);
	bot.dataS += len;
	return;
}

static const char *table_dec = "0123456789";

int osntc(aNick *nick, const char *format, ...)
{	/* APACS N YYXXX :f*/
	static char buf[512];
	register char *p = buf;
	register const char *fmt = format;
	char t, *end = p + 509;
	int len = 0;
	va_list vl;

	va_start(vl, format);

	if(buf[0] == 0)
	{
		strcpy(p, os.num);
		p[5] = p[7] = p[13] = ' ';
		p[6] = TOKEN_NOTICE[0];
		p[14] = ':'; /* prefix, cmd, cible, parser formaté */
	}
	p += 15;
	buf[8] = nick->numeric[0];
	buf[9] = nick->numeric[1];
	buf[10] = nick->numeric[2];
	buf[11] = nick->numeric[3];
	buf[12] = nick->numeric[4];

	while((t = *fmt++) && p < end)/* %sa (t = %, *pattern = s) */
	{
		if(t == '%')
		{
			t = *fmt++;/* on drop le formateur (t = s, *pattern = a) */
			if(t == 's')
			{	/* copie de la string*/
				register char *tmps = va_arg(vl, char *);
				while(*tmps) *p++ = *tmps++;
				continue;
			}
			if(t == 'd')
			{
				int tmpi = va_arg(vl, int), pos = 31;
				char bufi[32];

				if(tmpi <= 0)
				{
					if(!tmpi)
					{
						*p++ = '0';
						continue;
					}
					*p++ = '-';
					tmpi = -tmpi;
				}
				while(tmpi)/* on converti une int en base 10 en string */
				{		/* écriture dans l'ordre inverse 51 > '   1' > '  51'*/
					bufi[pos--] = table_dec[tmpi % 10];
					tmpi /= 10;
				}
				while(pos < 31) *p++ = bufi[++pos];
				continue;
			}
			if(t == 'c')
			{
				*p++ = (char) va_arg(vl, int);
				continue;
			}
			if(t != '%')
			{	/* on sous traite le reste à vsnprintf */
				int i = vsnprintf(p, sizeof buf - (p - end + 509), fmt - 2, vl);/* on remet le %format */
				p += i > 0 ? i : sizeof buf - (p - end + 509);
				break;
			}
		}
		*p++ = t;
	}
	*p++ = '\r';
	*p++ = '\n';
	*p = '\0';

	len = p - end + 509;
	send(bot.sock, buf, len, 0);
	bot.dataS += len;
	va_end(vl);
	return 0;
}

void putchan(const char *buf)
{
	time_t tmt = CurrentTS;
	struct tm *ntime = localtime(&tmt);

	putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] %s", os.num, bot.pchan,
		ntime->tm_hour,	ntime->tm_min, ntime->tm_sec, buf);
	return;
}

void oswallops(const char *pattern, ...)
{
	char buf[350];
	va_list vl;

	va_start(vl, pattern);
	vsnprintf(buf, sizeof buf, pattern, vl);
	va_end(vl);
	putserv("%s " TOKEN_WALLOPS " :%s", os.num, buf);
}
