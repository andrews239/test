
#define MAXNODES 1024
#define MAXPORTS 10
#define PKTLOAD 40
#define MAXETHER 16384

// x2
#define CLIENTCHECK 30


#define DEVMODE_NEW 1
#define DEVMODE_CLIENT 2
#define DEVMODE_MASTER 3
#define DEVMODE_ELECT 4

char *devmodename[]={
    "null",
    "NEW","CLIENT","MASTER","ELECT"};

char *devmodenames[]={
    "0",
    "N","C","M","E"};

struct frame
  {
    int device_id;
    int port_id;
    int src_addr;
    int dst_addr;
    int ttl;
    int data[PKTLOAD];
  };

struct addrport
  {
    int addr;
    int port;
  };

struct client
  {
    int addr;
    int temp;
    int light;
    int last;
    int weight;
  };

struct device
  {
    void (*input)(struct device *, struct frame);
    int id;
    int type;
    int devmode;
    int serial;
// Endpoint
    int location_x;
    int location_y;
    int direction;
    int addr;
    int prio;
    int master_color;
    int master_addr;
    int master_prio;
    int master_last;
    int main_jobs;
    int disp_temp;
    int disp_light;
    struct client child[MAXNODES];
    int child_num;
    int child_fail;
// SW
    struct addrport addrtbl[MAXNODES];
    int packets;
    int bpackets;
    int portfail[MAXPORTS];
    int swfail;
    int port_dev[MAXPORTS];
    int port_port[MAXPORTS];
 };


struct job
  {
    int start_at;
    void (*call)(struct device *);
    struct device *param;
    int serial;
    int devmode;
  };

