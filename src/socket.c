/* src/socket.c - Gestion des sockets & parse
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
 * $Id: socket.c,v 1.3 2005/05/07 22:41:13 bugs Exp $
 */

#include <unistd.h>
#include "main.h"
#include "outils.h"
#include "serveur.h"
#include "socket.h"
#include "hash.h"
#include "debug.h"
#include "os_cmds.h"
#include "add_info.h"
#include "timers.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

fd_set global_fd_set; 
int highsock = 0; 

static struct aMsg {
  const char *cmd;
  int (*func) (int parc, char **parv);
} msgtab[] = {
  {TOKEN_PRIVMSG, m_privmsg},
  {TOKEN_JOIN, m_join},
  {TOKEN_NICK, m_nick},
  {TOKEN_QUIT, m_quit},
  {TOKEN_MODE, m_mode},
  {TOKEN_PART, m_part},
  {TOKEN_KICK, m_kick},
  {TOKEN_CREATE, m_create},
  {TOKEN_BURST, m_burst},
  {TOKEN_AWAY, m_away},
  {TOKEN_PING, m_ping},
  {"SERVER", m_server},
  {"OM", m_mode},
  {"CM", m_clearmode},
  {TOKEN_TOPIC, m_topic},
  {TOKEN_KILL, m_kill},
  {TOKEN_WHOIS, m_whois},
  {TOKEN_SERVER, m_server},
  {TOKEN_SQUIT, m_squit},
  {TOKEN_EOB, m_eob},
  {TOKEN_PASS, m_pass},
  {TOKEN_ACCOUNT, m_account},
  {"ERROR", m_error},
};

static void parse_this(char *msg)
{
	int parc = 0;
	unsigned int i=0;
	char *cmd = NULL, *parv[MAXPARA + 2];
#ifdef DEBUG
	char logg[513];
	Strncpy(logg, msg, 512);
#endif

	if(!mainhub) parv[parc++] = bot.server;

	while(*msg)
	{
		while(*msg == ' ') *msg++ = 0;/* on supprime les espaces pour le découpage*/
		if(*msg == ':') /* last param*/
		{
			parv[parc++] = msg + 1;
			break;
		}
		if(!*msg) break;

		if(parc == 1 && !cmd) cmd = msg;/* premier passage à parc 1 > cmd*/
		else parv[parc++] = msg;
		while(*msg && *msg != ' ') msg++;/*on laisse passer l'arg..*/
	}
	parv[parc] = NULL;

#ifdef DEBUG /*etical loging*/
	if(parc <= 1 || strcasecmp(cmd, TOKEN_PRIVMSG) || *parv[1] != '#')
		putlog(LOG_PARSES, "R - %s", logg);
#endif

	for(;i < ASIZE(msgtab);++i)
		if (!strcasecmp(msgtab[i].cmd, cmd))
		{
			msgtab[i].func(parc, parv);
			return;/*on sait jamais */
		}
}

int init_bot(unsigned long int ip)
{
	time_t LTS = time(NULL);
	struct sockaddr_in fsocket;
	char num[7];
	int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP), opts;

	if((bot.sock = sock) < 0) return -1;

	fsocket.sin_family = AF_INET;
	fsocket.sin_addr.s_addr = ip;
	fsocket.sin_port = htons(bot.port);

	if(connect(sock, (struct sockaddr *) &fsocket, sizeof fsocket) == -1)
	{
		Debug(W_MAX, "Erreur #%d lors de la connexion: %s", errno, strerror(errno));
		close(sock);
		return -1;
	}

	SOCK_REGISTER(sock);
	if((opts = fcntl(sock, F_GETFL)) < 0) Debug(W_MAX, "Erreur lors de fcntl(F_GETFL)");
	else if(fcntl(sock, F_SETFL, opts | O_NONBLOCK) < 0)
		Debug(W_MAX, "Erreur lors de fcntl(F_SETFL)");
	mainhub = NULL;
	putserv("PASS %s", bot.pass);
	sprintf(num, "%s]]]", bot.servnum);

	putserv("SERVER %s 1 %lu %lu J10 %s +s :%s", bot.server, bot.uptime, LTS, num, bot.name);
	add_server(bot.server, num, "1", "J10", bot.server);

	putserv("%s "TOKEN_NICK" %s 1 %lu %s %s %s B]AAAB %s :%s",
		bot.servnum, os.nick, LTS, os.ident, os.host, os.mode, os.num, os.name);
	add_nickinfo(os.nick, os.ident, os.host, "B]AAAB", os.num,
		num2servinfo(bot.servnum), os.name, LTS, os.mode);

        return sock;
}

int run_bot(int sock)
{
	char tbuf[513] = {0}; /* IRC buf */
	int t = 0, read, i;
	char bbuf[TCPWINDOWSIZE];/* fonction commune de parsage de l'ircsock*/
	register char *ptr;

        fd_set tmp_fdset;
	struct timeval timeout = {0};
	time_t last_timer = 0;

	while(running)
	{
		CurrentTS = time(NULL);

		if(CurrentTS - last_timer >= ALARMINTERVAL)
		{
                        exec_timers(); /* exec timers */ 
                        last_timer = CurrentTS; 
                } 

                /* update timeout to remaining time */ 
                timeout.tv_sec = ALARMINTERVAL - (CurrentTS - last_timer); 
                timeout.tv_usec = 0; 

                /* save fd set before select */ 
                tmp_fdset = global_fd_set;

                if(select(highsock + 1, &tmp_fdset, NULL, NULL, &timeout) < 0) 
                {
                        if(errno != EINTR)
                        {
                                Debug(W_MAX|W_WARN, "Erreur lors de select() (%d: %s)", errno, strerror(errno));
                                exit(EXIT_FAILURE);
                        }
                }
                else
                {
                        for(i = 0;i <= highsock;++i)
                        {
                                if(!FD_ISSET(i, &tmp_fdset)) continue;
                                if(i == bot.sock)
                                {
					/* lecture du buffer de recv(), puis découpage selon les \r\n et,
					enfin on garde en mémoire dans tbuf le début d'un paquet IRC s'il
					est incomplet. au select suivant les bytes lui appartenant lui seront
					ajoutée grace à t, index permanent.       packet1\r\npacket2\r\n
					la lecture de bloc de 512 bytes plutot que byte à byte est
					au moins 6 fois plus rapide!*/
					read = recv(sock, bbuf, sizeof bbuf -1, 0);
					ptr = bbuf;

					if(read <= 0 && errno != EINTR)
					{
						close(sock);
						return Debug(W_MAX, "Erreur lors de recv(): [%s]", strerror(errno));
					}

					bbuf[read] = 0;
					bot.dataQ += read; /* compteur du traffic incoming*/
		
					while(*ptr)
					{/* si ircd compatible, \r avant \n..*/
						if(*ptr == '\n') 		/* fin de la ligne courante*/
						{
							tbuf[t-1] = 0;		/* efface le \r*/
							parse_this(tbuf);
							t = 0;				/* index du buffer permanent*/
						}
						else tbuf[t++] = *ptr;	/* copie dans le buffer permanent */
						++ptr;					/* qui garde la ligne courante*/
					}
                                }
                        } /* for(highsock) */
                } /* select() */

	} /* end main loop */

	if(!running)
	{
		close(sock);
	}
	bot.sock = 0;
	return 0;
}
