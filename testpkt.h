#define PKT_PING 1
#define PKT_PONG 2
#define PKT_GETMASTER 3
#define PKT_MASTER 4
#define PKT_ELECT_START 5
#define PKT_ELECT 6
#define PKT_GET 7
#define PKT_RPL 8
#define PKT_SET 9
#define PKT_LAST 9


char *pktname[]={
    "null",
    "ping","pong",
    "getmaster","master_ready",
    "elect_start","elect",
    "GET","SET"};
