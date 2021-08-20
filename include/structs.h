/* include/structs.h - Déclaration des différentes structures
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
 */

/*------------------- Structures robot -------------------*/
struct robot {
  char server[HOSTLEN + 1];
  char servnum[NUMSERV + 1];
  char pass[21];
  int port;
  char name[REALEN + 1];
  char ip[16];
  char chan[REGCHANLEN + 1];
  char chanadmin[REGCHANLEN + 1];
  char pchan[REGCHANLEN + 1];
  char cara;
  int sock;
  time_t uptime;
  time_t lasttime;	/* pour traffic */
  unsigned long int lastbytes;  /*  idem */
  unsigned long int dataS; /* bytes envoyées depuis le début*/
  unsigned long int dataQ; /* bytes reçues*/

};

struct bots {
  char nick[NICKLEN + 1];
  char ident[USERLEN + 1];
  char host[HOSTLEN + 1];
  char name[REALEN + 1];
  char mode[10];
  char num[2 * (NUMSERV + 1 )];
};

/*------------------- Structures informations serveurs -------------------*/

typedef struct Link {
	char serv[HOSTLEN + 1];
	char num[NUMSERV + 1];
	int maxusers;
	int smask;
	int flag;
#define ST_BURST 	0x01
#define ST_ONLINE 	0x02
#define ISHUB 		0x04
	struct Link *hub;
} aServer;

/*------------------- Structures users et accès -------------------*/
typedef struct nickinfo {
	char nick[NICKLEN + 1];
	char ident[USERLEN + 1];
	char host[HOSTLEN + 1];

	struct Link *serveur;
	char name[REALEN + 1];
	char base64[7];
	char numeric[2 * (NUMSERV + 1 )];
	unsigned int flag;
#define N_REGISTER 	0x000001
#define N_AWAY 		0x000002
#define N_OPER 		0x000004
#define N_SERVICE 	0x000008
#define N_ADM 		0x000010
#define N_INV 		0x000020
#define N_GOD 		0x000040
#define N_WALLOPS 	0x000080
#define N_DEBUG 	0x000100
#define N_FEMME 	0x000200
#define N_HOMME 	0x000400
#define N_CRYPT 	0x000800
#define N_REG 		0x001000
#define N_DIE 		0x002000
#define N_DEAF 		0x004000
#define N_SPOOF 	0x008000
#define N_HELPER	0x010000
#define N_WHOIS		0x020000
#define N_IDLE		0x040000
#define N_CHANNEL	0x080000
#define N_PRIVATE	0x100000
#define N_HIDE		0x200000
#define N_HASKILL   0x400000

#define N_UMODES (N_INV |N_CRYPT|N_REG|N_OPER|N_SERVICE|N_ADM|N_GOD \
	|N_FEMME|N_HOMME|N_DEBUG|N_WALLOPS|N_DIE|N_DEAF|N_SPOOF|N_HELPER \
	|N_WHOIS|N_IDLE|N_CHANNEL|N_PRIVATE|N_HIDE)

	time_t ttmco;
	time_t floodtime;
	unsigned int floodcount;
	struct userinfo *user;
	struct nickinfo *next;
	struct joininfo *joinhead;
} aNick;

typedef struct userinfo {
   char nick[NICKLEN + 1];
   int level;
   struct access *accesshead;
   aNick *n;
   struct userinfo *next;
} anUser;

typedef struct access {
   struct chaninfo *c; /* pour rediriger vers la struct du chan où on a access.*/
   int level;
   char *info;
   struct userinfo *user;
   struct access *next;
} anAccess;

struct trusted {
	char ip[16];
	struct trusted *next;
};

struct gline {
	char host[HOSTLEN + 1];
	int time;
	char raison[150];
	struct gline *next;
};

struct shun {
        char host[HOSTLEN + 1];
        int time;
        char raison[150];
        struct shun *next;
};

/*------------------- Structures chans -------------------*/

struct cmode {
#define C_MMSG 		0x0001
#define C_MTOPIC 	0x0002
#define C_MINV 		0x0004
#define C_MLIMIT 	0x0008
#define C_MKEY 		0x0010
#define C_MSECRET 	0x0020
#define C_MPRIVATE 	0x0040
#define C_MMODERATE 	0x0080
#define C_MNOCTRL 	0x0100
#define C_MNOCTCP 	0x0200
#define C_MOPERONLY 	0x0400
#define C_MUSERONLY 	0x0800
#define C_MACCONLY	0x1000
#define C_MHELPER	0x2000
#define C_MNONOTICE	0x4000
#define C_MNOQUITPARTS	0x8000
#define C_MSSLONLY	0x10000
#define C_MAUDITORIUM	0X20000

	unsigned int modes; /* modes sous forme de flag*/
	int limit;
	char key[KEYLEN + 1];
};

typedef struct chaninfo {
	char chan[REGCHANLEN + 1];/* infos sur le salon actuel du réseau*/
	char topic[TOPICLEN + 1];
	time_t chanstamp;
	struct cmode mode;
	int users;

	anAccess *owner;
	struct SLink *access; /* utiliser un link pour avoir la liste des access plus rapidement*/
	struct SLink *members; /* listes ds users dans le chan*/
	struct chaninfo *next;
} aChan;

typedef struct joininfo
{
   char channel[CHANLEN + 1];
   unsigned short status;
#define J_OP 	0x01
#define J_HALF	0X02
#define J_VOICE 0x04
#define J_BURST 0x08
   time_t timestamp;
   aNick *nick;
   struct chaninfo *chan;
   struct joininfo *next;
} aJoin;

typedef struct HelpBuf { 
        char **buf; 
        int count; 
} HelpBuf; 
    
/*------------------- Structures parses et cmds -------------------*/

typedef struct aHashCmd {
	char corename[CMDLEN + 1];
	char name[CMDLEN + 1];
	int (*func) (aNick *, aChan *, int, char **);
	char syntax[SYNTAXLEN + 1];
	int level;
	int args;
	int flag;
#define CMD_CHAN                0x01
#define CMD_ADMIN               0x02
#define CMD_DISABLE     	0x04
#define CMD_NEEDNOAUTH  	0x08
#define CMD_SECURE              0x10
#define CMD_SECURE2     	0x20
#define CMD_HELPER		0x40
#define CMD_INTERNAL (CMD_NEEDNOAUTH|CMD_SECURE|CMD_SECURE2|CMD_CHAN)
	int used;
	struct aHashCmd *next;
	HelpBuf **help;
} aHashCmd;

/*------------------- Structures pour les Links -------------------*/

typedef struct SLink {
  struct SLink *next;
  union {
    anAccess *a;
    aJoin *j;
  } value;
} aLink;

typedef struct aBAD_ {
        char *mask;
        char *raison;
        struct aBAD_ *next;
        struct aBAD_ *last;
        unsigned int flag;
#define BAD_TYPEUSER 0x01
#define BAD_TYPECHAN 0x02
#define BAD_MASK     0x04
        time_t date;
        char from[NICKLEN + 1];
} aBAD;

typedef struct aEX_ {
        char *mask;
        char *raison;
        struct aEX_ *next;
        struct aEX_ *last;
        unsigned int flag;
#define EX_TYPEUSER 0x01
#define EX_TYPECHAN 0x02
#define EX_MASK     0x04
        time_t date;
        char from[NICKLEN + 1];
} aEX;

struct trace {
        char mask[NUHLEN];
        char raison[150];
        struct trace *next;
};
