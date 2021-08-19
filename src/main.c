/* src/main.c - Fichier principal
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
 */

#include "main.h"
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#ifdef USEBSD
#include <netinet/in.h>
#endif
#include <arpa/inet.h>
#include <sys/stat.h>
#include "config.h"
#include "mylog.h"
#include "hash.h"
#include "outils.h"
#include "serveur.h"
#include "fichiers.h"
#include "chanserv.h"
#include "admin_user.h"
#include "admin_cmds.h"
#include "admin_chan.h"
#include "admin_manage.h"
#include "divers.h"
#include "opdeop.h"
#include "showcommands.h"
#include "aide.h"
#include "cs_register.h"
#include "cs_cmds.h"
#include "timers.h"
#include "socket.h"
#include "flood.h"
#include "version.h"

int running = 1;
time_t CurrentTS = 0;

Lang *DefaultLang = NULL;

struct robot bot = {{0}};
struct bots cs = {{0}};

aChan *chan_tab[CHANHASHSIZE] = {0}; 		/* hash chan */
anUser *user_tab[USERHASHSIZE] = {0}; 		/* hash username */
aNChan *nchan_tab[NCHANHASHSIZE] = {0}; 	/* hash nchan */
aNick **num_tab[MAXNUM] = {0}; 				/* table num -> struct nick */
aServer *serv_tab[MAXNUM] = {0}; 			/* table numserv -> serv info */
aNick *nick_tab[NICKHASHSIZE] = {0}; 		/* hash nick */


static void sig_die(int c)
{
	log_write(LOG_MAIN, LOG_DOWALLOPS, "SIGTERM reçu! -- abandon..");
	running = 0;
}

static void sig_restart(int c)
{
	log_write(LOG_MAIN, LOG_DOWALLOPS, "SIGINT reçu! -- Restarting..");
	running = 0;
	ConfFlag |= CF_RESTART;

	putserv("%s "TOKEN_QUIT" :Restarting", cs.num);
	putserv("%s "TOKEN_SQUIT" %s 0 :SIGINT/RESTART", bot.servnum, bot.server);
}

static void sig_reload(int c)
{
	log_write(LOG_MAIN, LOG_DOWALLOPS, "Signal HUP reçu!");
	signal(SIGHUP, &sig_reload);
}

static void pid_write(void)
{
	FILE *fd = fopen(SDREAMS_PID, "w");

	if(fd)
	{
		fprintf(fd, "%d\n", (int) getpid());
		fclose(fd);
	}
	else log_write(LOG_MAIN, LOG_DOTTY, "Impossible d'écrire le fichier PID. [%s]",
			strerror(errno));
}

int main(int argc, char **argv)
{
	int silent = 0, tmp = 0, background = 1;
	FILE *fd; /* uptime */
    struct rlimit rlim; /* used for core size */

	CurrentTS = time(NULL);

	while((tmp = getopt(argc, argv, "hvn")) != EOF)
		switch(tmp)
		{
			case 'n':
				background = 0;
				break;
			case 'h':
				silent = 1;
				break;
			case 'v':
				puts("Services ODreams " SPVERSION " (Rev:" REVDATE")\n"
					" (Build " __DATE__ " "__TIME__ ")\n"
					"© 2021 @bugsounet");
				return 0;
			default:
				printf("Syntax: %s [-hvn]\n", argv[0]);
				exit(EXIT_FAILURE);
		}

	chdir(BINDIR); 		/* going in our main base directory */
	umask(077); 		/* to prevent our files from being accessible by other */
	srand(CurrentTS); 	/* randomize the seed */

	/* Hack to check if another instance is currently running (FIX? lock) */
	if((fd = fopen(SDREAMS_PID, "r")))
	{
		if(fscanf(fd, "%d", &tmp) == 1)
		{
			fprintf(stderr, "ODreams est déjà lancé sur le pid %d.\nSi ce n'est pas "
				"le cas, supprimez le fichier '"SDREAMS_PID"' et recommencez.\n", tmp);
		}
		fclose(fd);
		exit(EXIT_FAILURE);
	}

	if(load_config(FICHIER_CONF) == -1)
	{
		fputs("Erreur lors de la lecture de la configuration\n", stderr);
		exit(EXIT_FAILURE);
	}

	/* Is default language complete ? */
	if(!lang_check_default()) exit(EXIT_FAILURE);

	if((fd = fopen("uptime.tmp", "r")))
	{
		fscanf(fd, "%lu", (unsigned long *) &bot.uptime);
		fclose(fd);
		remove("uptime.tmp");
		silent = 1;
	}
	else bot.uptime = CurrentTS;

	RegisterCmd("REGISTER", 	0, CMD_NEEDNOAUTH | CMD_SECURE, 3, first_register);
	//RegisterCmd("USER", 		2, CMD_ADMIN | CMD_SECURE3, 2, admin_user);
	RegisterCmd("DIE", 			7, CMD_ADMIN, 0, die);
	RegisterCmd("REHASH",		7, CMD_ADMIN, 0, rehash_conf);
	RegisterCmd("RESTART", 		7, CMD_ADMIN, 0, restart_bot);
	RegisterCmd("SHOWCOMMANDS", 0, CMD_NEEDNOAUTH, 0, showcommands);
	/*
	RegisterCmd("CHCOMNAME", 	6, CMD_ADMIN, 2, chcomname);
	RegisterCmd("ADMINLVL", 	6, CMD_ADMIN, 2, admin_level);
	RegisterCmd("CHLEVEL", 		6, CMD_ADMIN, 2, chlevel);
	RegisterCmd("DISABLE", 		6, CMD_ADMIN, 2, disable_cmd);
	RegisterCmd("INVITEME",  	3, CMD_ADMIN, 0, inviteme);
	RegisterCmd("WRITE", 		6, CMD_ADMIN, 0, write_files);
	RegisterCmd("WHOIS", 		3, CMD_NEEDNOAUTH|CMD_ADMIN, 1, cs_whois);
	RegisterCmd("SHOWCONFIG", 	7, CMD_NEEDNOAUTH|CMD_ADMIN, 0, showconfig);
	RegisterCmd("GLOBAL", 		5, CMD_ADMIN, 2, globals_cmds);
	RegisterCmd("HELP", 		0, CMD_NEEDNOAUTH, 0, aide);
	RegisterCmd("ADMIN", 		0, CMD_NEEDNOAUTH, 0, show_admins);
	RegisterCmd("UPTIME", 		0, CMD_NEEDNOAUTH, 0, uptime);
	RegisterCmd("\1ping", 		0, CMD_NEEDNOAUTH, 0, ctcp_ping);
	RegisterCmd("\1version\1", 	0, CMD_NEEDNOAUTH, 0, ctcp_version);

	RegisterCmd("DNRCHAN", 		4, CMD_ADMIN, 1, dnrchan_manage);
	RegisterCmd("DNRUSER", 		4, CMD_ADMIN, 1, dnruser_manage);
	RegisterCmd("WHOISON", 		2, CMD_ADMIN|CMD_CHAN|CMD_MBRSHIP, 0, whoison);
	RegisterCmd("SAY", 			4, CMD_ADMIN|CMD_CHAN|CMD_MBRSHIP, 2, admin_say);
	RegisterCmd("DO", 			4, CMD_ADMIN|CMD_CHAN|CMD_MBRSHIP, 2, admin_do);
    */
	if(!silent) puts("Services ODreams " SPVERSION " v2 © 2021");

	db_load_users(silent); 	/* so load_users() will manage to add accesses */

	if(!GetConf(CF_PREMIERE))
	{
		load_cmds(silent);
	} /* first time */

	BuildCommandsTable(0);
	help_load(NULL); 	/* load all languages */

	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP,	&sig_reload);
	signal(SIGINT, 	&sig_restart);
	signal(SIGTERM, &sig_die);

	if(background && (tmp = fork()) == -1)
	{
		log_write(LOG_MAIN, LOG_DOTTY, "Impossible de se lancer en background. (#%d: %s)",
			errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	else if(background && tmp > 1) /* fork ok */
	{
		if(!silent) puts("Lancement en background...");
		exit(0);
	}

	if(!getrlimit(RLIMIT_CORE, &rlim) && rlim.rlim_cur != RLIM_INFINITY)
	{
		log_write(LOG_MAIN, LOG_DOTTY, "Core size limitée à %Uk, changement en illimité.",
			rlim.rlim_cur);
		rlim.rlim_cur = RLIM_INFINITY;
		rlim.rlim_max = RLIM_INFINITY;
		setrlimit(RLIMIT_CORE, &rlim);
	}

	if(GetConf(CF_PREMIERE)) printf("Lorsque les services seront sur votre réseau IRC,\n"
		"tapez : /%s %s <username> <email> <email> <pass>\n", cs.nick, RealCmd("register"));

	if(!fd_in_use(0)) close(0); /* closing std(in|out|err) to free up some fds */
	if(background && !fd_in_use(1)) close(1);

	pid_write();

	timer_add(PURGEDELAY, TIMER_PERIODIC, callback_check_accounts, NULL, NULL);
	timer_add(cf_write_delay, TIMER_PERIODIC, callback_write_dbs, NULL, NULL);

	socket_init();
	init_bot(); /* 1st main socket initialization */
	run_bot(); /* main loop */

	if(!GetConf(CF_PREMIERE))
	{
		db_write_users();
		write_cmds();
	}

	log_write(LOG_MAIN, 0, "Fermeture du programme%s",
		GetConf(CF_RESTART) ? " (Restarting)" : "");

	remove(SDREAMS_PID);
	CleanUp();

	if(GetConf(CF_RESTART)) execlp(argv[0], argv[0], "-h", NULL); /* restarting.. */
	return 0;
}
