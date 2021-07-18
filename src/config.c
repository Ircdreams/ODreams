/* src/fichiers.c - Lecture/Écriture des données
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
 * $Id: config.c,v 1.8 2005/12/06 16:37:09 bugs Exp $
 */

#include "main.h"
#include "config.h"
#include "outils.h"
#include <errno.h>

int clonemax = 10;
char *blockmsg = NULL;
char *shunmsg = NULL;
char *defraison = NULL;
char *quit_msg = NULL;
char *pasdeperm = NULL;
char *network = NULL;
char *chnick = NULL;

#define TabId(tab) (tab->id - CONF_IREAD)

enum conf_id {CONF_UPLINK = 0, CONF_OSBOT, CONF_MYSERV, CONF_MISC, CONF_IREAD};

struct conf_item {
	enum conf_id tabid;
	int flag;
#define CONF_READ 	0x001
#define CONF_TINT 	0x002
#define CONF_PORT 	0x004
#define CONF_TPTR 	0x008
#define CONF_TARRAY 	0x010
#define CONF_IP 	0x020
#define CONF_TDUR 	0x040
#define CONF_TPRIV 	0x080
#define CONF_TFLAG 	0x100
	void *ptr;
#define CONF_MARRAY(x) x, sizeof x - 1
	size_t psize;
	const char *item;
	const char *description;
	int (*cf_valid)(struct conf_item *, char *);
};

static struct conf_tab {
	const char *item;
	const char *description;
	int id;
} conftab[] = {
	{"uplink", "configuration des infos nécessaire au link (ip, port)", CONF_UPLINK},
	{"osbot", "configuration des infos du Channel Service", CONF_OSBOT},
	{"myserver", "configuration des infos du serveur du OS", CONF_MYSERV},
	{"misc_conf", "configuration des diverses options (On The Fly!) des Services", CONF_MISC},
};

static int cf_validnick(struct conf_item *i, char *buf)
{
	char *p = (char *) i->ptr;
	if(IsValidNick(p)) return 1;
	snprintf(buf, 300, "'%s' n'est pas un nick valide.", p);
	return 0;
}

static int cf_validchan(struct conf_item *i, char *buf)
{
	char *p = (char *) i->ptr;
	if(*p == '#') return 1;
	snprintf(buf, 300, "'%s' n'est pas un nom de salon valide.", p);
	return 0;
}

static int cf_validserv(struct conf_item *i, char *buf)
{
	if(strchr((char *) i->ptr, '.')) return 1;
	snprintf(buf, 300, "item %s: le nom du serveur doit contenir un '.'.", i->item);
	return 0;
}

static int cf_validport(struct conf_item *i, char *buf)
{
	int port = *(int *) i->ptr;
	if(port > 0 && port <= 65535) return 1;
	snprintf(buf, 300, "item %s: %d n'est pas un port valide (1-65535).", i->item, port);
	return 0;
}

static int cf_validnum(struct conf_item *i, char *buf)
{
	const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789[]";
	int c = atoi((char *) i->ptr), n = 0;

	if(c >  MAXNUM)
	{
		snprintf(buf, 300, "item %s: Numeric non valide %d (0-%d)",	i->item, c , MAXNUM);
		return 0;
	}
	bot.servnum[n++] = base64[c / 64];
	bot.servnum[n++] = base64[c & 63];
	bot.servnum[n] = '\0';
	return 1;
}

struct conf_item conf_items[] = {
	/* uplink */
	{CONF_UPLINK, CONF_IP|CONF_TARRAY, CONF_MARRAY(bot.ip), "ip",
		"IP du HUB auquel le serveur doit se connecter", NULL},
	{CONF_UPLINK, CONF_TARRAY, CONF_MARRAY(bot.pass), "pass", "Pass du link", NULL},
	{CONF_UPLINK, CONF_TINT|CONF_PORT, &bot.port, 0, "port",
		"Port du HUB auquel le serveur doit se connecter", cf_validport},
	/* osbot */
	{CONF_OSBOT, CONF_TARRAY, CONF_MARRAY(os.nick), "nick", "Pseudo du OS", cf_validnick},
	{CONF_OSBOT, CONF_TARRAY, CONF_MARRAY(os.ident), "ident", "Ident du OS", NULL},
	{CONF_OSBOT, CONF_TARRAY, CONF_MARRAY(os.host), "host", "Host du OS", NULL},
	{CONF_OSBOT, CONF_TARRAY, CONF_MARRAY(os.name), "realname", "Real Name du OS", NULL},
	{CONF_OSBOT, CONF_TARRAY, CONF_MARRAY(os.mode), "modes",
		"UserModes que doit porter le OS à la connexion (+ok normalement)", NULL},
	{CONF_OSBOT, CONF_TARRAY, CONF_MARRAY(bot.pchan), "chan",
		"Salon de log où est envoyée l'activité du OS", cf_validchan},
	/* myserv */
	{CONF_MYSERV, CONF_TARRAY, CONF_MARRAY(bot.server), "server",
		"Nom littéral du serveur", cf_validserv},
	{CONF_MYSERV, CONF_TARRAY, CONF_MARRAY(bot.name), "infos",
		"Infos du serveur, visible dans le /links", NULL},
	{CONF_MYSERV, CONF_TPRIV|CONF_TINT, NULL, 0, "numeric",
		"Numeric P10 du serveur (en chiffre)", cf_validnum},
	{CONF_MYSERV, CONF_TPTR, &network, 0, "network", "Nom du réseau", NULL},
	{CONF_MYSERV, CONF_TPTR, &quit_msg, 0, "quit_msg",
		"Message envoyé lors de la maintenance des services", NULL},
	/* misc */
	{CONF_MISC, CONF_TPTR, &pasdeperm, 0, "pas_de_perm",
		 "message envoyé à un user utilisant une commande nécessitant d'être logué.", NULL},
	{CONF_MISC, CONF_TPTR, &defraison, 0, "defraison",
                "Raison par default des Kicks / Glines / Kills / Shuns", NULL},
	
	 /* Operserv */
        {CONF_MISC, CONF_TINT, &clonemax, 0, "maxclones",
                "Nombre de clones maximum autorisé", NULL},
        {CONF_MISC, CONF_TPTR, &blockmsg, 0, "blockmsg",
                "Message du G-line en cas d'utilisation de la fonction Block", NULL},
	{CONF_MISC, CONF_TPTR, &shunmsg, 0, "shunmsg",
                "Message du Shun en cas d'utilisation de la fonction Silence", NULL},
	{CONF_MISC, CONF_TPTR, &chnick, 0, "chnick",
		"Préfix mis devant les chiffres aléatoires des pseudos donnés par le robot lors de la prise d'un pseudo interdit", NULL},
};

static struct conf_tab *get_tab(const char *item)
{
	unsigned int i = 0;
	for(;i < ASIZE(conftab);++i) if(!strcasecmp(conftab[i].item, item))
	{
		if(conftab[i].id < CONF_IREAD) conftab[i].id += CONF_IREAD;
		return &conftab[i];
	}
	return NULL;
}

static int get_item(struct conf_tab *tab, const char *item)
{
	unsigned int i = 0;
	
	for(;i < ASIZE(conf_items) && conf_items[i].tabid <= TabId(tab);++i)
		 if(conf_items[i].tabid == TabId(tab) && !strcasecmp(conf_items[i].item, item)) return i;
	return -1;
}

static int check_missing_item(struct conf_tab *tab)
{
	unsigned int i = 0;

	for(;i < ASIZE(conf_items) && conf_items[i].tabid <= TabId(tab);++i)
	{
		struct conf_item *item = &conf_items[i];
		if(item->tabid == TabId(tab) && !(item->flag & CONF_READ))
		{
			printf("conf -- Table %s, item '%s' manquant (%s)\n", tab->item, item->item, item->description);
			return 1;
		}
	}
	return 0;
}

static char *conf_parse(char *ptr, struct conf_tab *tab, struct conf_item *item)
{
	ptr = strtok(NULL, "=\r\n");
	while(ptr && *ptr && (*ptr == ' ' || *ptr == '\t')) ++ptr;
	if(!ptr || !*ptr)
	{
		printf("conf -- Table '%s', item '%s' sans valeur.\n", tab->item, item->item);
		return NULL;
	}
	item->flag |= CONF_READ;
	return ptr;
}

static int readconf(FILE *fp)
{
	int line = 0, current_item = -1, i = -1;
	struct conf_item *citem = NULL;
	char buf[512], vreason[300];
	struct conf_tab *curtab = NULL;

	while(fgets(buf, sizeof buf, fp))
	{
		char *ptr = buf;
		++line;

		while(*ptr == '\t' || *ptr == ' ') ptr++; /* on saute les espaces/tabs */
		if(*ptr == '#' || *ptr == '\r' || *ptr == '\n' || !*ptr) continue; /* commentaire OU ligne vide */
		ptr = strtok(ptr, " ");

		if(i != -2 && !curtab) /* no tab selected! */
		{
			if(!(curtab = get_tab(ptr)))
				printf("conf -- Ligne: %d, table inconnue: %s\n", line, ptr), i = -2;
			continue;
		}

		if(EndOfTab(ptr)) /* maybe I'm just at the end '}' */
		{
			if(curtab && check_missing_item(curtab)) return -1; /* end of /tab */
			citem = NULL;
			curtab = NULL;
			current_item = i = -1;
			continue;
		}
		if(i == -2) continue;
		/* enforce some type checks */

		if((current_item = get_item(curtab, ptr)) == -1)
		{
			printf("conf -- Ligne: %d, table %s, item '%s' inconnu.\n", line, curtab->item, ptr);
			continue;
		}
		else if(!(ptr = conf_parse(ptr, curtab, (citem = &conf_items[current_item])))) return -1;

		if(citem->flag & CONF_TINT && !is_num(ptr))
		{
			printf("conf -- Ligne: %d, table %s, l'item '%s' doit être un nombre.\n",
				 line, curtab->item, citem->item);
			return -1;
		}
		if(citem->flag & CONF_IP && !is_ip(ptr))
		{
			printf("conf -- Ligne: %d, table %s, item %s: %s ne semble pas être une IP valide.\n",
				line, curtab->item, citem->item, ptr);
			return -1;
		}
		/* now copy and transtype the data */
		if(citem->flag & CONF_TPRIV) /* ok let's its function handle the whole stuff */
		{
			citem->ptr = ptr; /* pass the value via the void * */
			if(citem->cf_valid && !citem->cf_valid(citem, vreason))
			{
				printf("conf -- Ligne: %d, table %s, %s\n", line, curtab->item, vreason);
				return -1;
			}
		}
		else if(citem->flag & CONF_TARRAY)
			Strncpy((char *) citem->ptr, ptr, citem->psize);
		else if(citem->flag & CONF_TPTR)
			str_dup((char **) citem->ptr, ptr);
		else if(citem->flag & CONF_TFLAG)
		{
			if(atoi(ptr)) *(int *)citem->ptr |= citem->psize;
			else *(int *)citem->ptr &= ~citem->psize;
		}
		else if(citem->flag & CONF_TINT)
			*(int *)citem->ptr = atoi(ptr);
		else if(citem->flag & CONF_TDUR)
		{
			if((*(int *)citem->ptr = convert_duration(ptr)) <= 0)
			{
				printf("conf -- Ligne: %d, table %s, item %s: '%s' n'est pas un format de durée valide.\n",
					line, curtab->item, citem->item, ptr);
				return -1;
			}
		}
		else printf("conf -- Ligne: %d, table %s, l'item '%s' n'a pas de type défini!!\n",
			line, curtab->item, citem->item);
		/* more check */

		if(citem->cf_valid && !citem->cf_valid(citem, vreason))
		{
			printf("conf -- Ligne: %d, table %s, %s\n", line, curtab->item, vreason);
			return -1;
		}
	}

	if(curtab)
	{
		printf("conf -- Erreur, parsage incomplet, manque '}', derniere table : %s\n", curtab->item);
		return -1;
	}
	else for(i = 0;i < ASIZE(conftab);++i) if(conftab[i].id < CONF_IREAD)
	{
		printf("conf -- Table '%s' manquante (%s).\n", conftab[i].item, conftab[i].description);
		return -1;
	}
	return 1;
}

int load_config(const char *file)
{
	FILE *fp = fopen(file, "r");
	int error = -1;
	if(fp)
	{
	error = readconf(fp);
	fclose(fp);
	}
	else printf("conf: Impossible d'ouvrir le fichier de conf (%s)", strerror(errno));
	return error;
}
