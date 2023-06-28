//typedefs that will be used throughout 

typedef struct _PLAYERINFO {
	char playername[32];
	char orgname[32];
	int ping;
	int score;
} PLAYERINFO;

typedef struct _SERVERINFO {
	unsigned int ip;
	unsigned short port;
	char szHostName[256];
	char szMapName[256];
	char szAdmin[256];
	char szWebSite[256];
	char szFragLimit[16];
	char szTimeLimit[16];
	char szVersion[64];
	int curClients;
	int maxClients;
	DWORD startPing;
	int	ping;
	int responded;
	PLAYERINFO players[64];
	unsigned int socket;
} SERVERINFO;

typedef struct _SERVERLIST {
		char name[128];
		char ipstring[24];
		char map[32];
		int real_pos;
		int ping;
		int players;
		PLAYERINFO player[64];
		unsigned int ip;
		bool sorted;
} SERVERLIST; 