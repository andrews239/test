#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>



#include "test.h"
#include "testeth.h"
#include "testjobs.h"
#include "testpkt.h"
#include "testshow.h"
#include "testsw.h"


struct device nodes[MAXNODES];

void runq()
  {
    cur = time(0);
    while(ether_ready())
      {
        struct frame fr = ether_get();
        (*nodes[fr.device_id].input)(nodes+fr.device_id, fr);
        dev_print(nodes+fr.device_id);
      }
    while(jobs_ready())
      {
        struct job jb = jobs_get();
        if( (jb.serial != jb.param->serial) || (jb.devmode != jb.param->devmode) )
          {
            continue;
          }
        (*jb.call)(jb.param);
        dev_print(jb.param);
        while(ether_ready())
          {
            struct frame fr = ether_get();
            (*nodes[fr.device_id].input)(nodes+fr.device_id, fr);
            dev_print(nodes+fr.device_id);
          }
      }
  }

void dev_addchild(struct device *dev, int addr, int new_prio)
  {
    for(int i=0; i < dev->child_num; i++)
      {
        if(dev->child[i].addr == addr) return;
      }
    dev->child[dev->child_num].addr = addr;
    dev->child[dev->child_num].last = 0;
    dev->child[dev->child_num].weight = 0;

    dev->child_num++;

    if(new_prio > dev->master_prio) {
// //       printf("%d : change %d device (addr=%d) master addr from %d to %d\n", cur-start, dev->id, dev->addr, dev->master_addr, addr);
        dev->master_addr = addr;
        dev->master_prio = new_prio;
    }
  }

void dev_updatechild(struct device *dev, int addr, int light, int temp)
  {
    for(int i=0; i < dev->child_num; i++)
      {
        if(dev->child[i].addr == addr)
          {
            dev->child[i].light = light;
            dev->child[i].temp = temp;
            dev->child[i].last = cur;
            dev->child[i].weight = 16;
            return;
          }
      }
  }

void devmode_set(struct device *dev,int mode)
  {
// //    printf("%d : change %d device (addr=%d) mode from %s to %s\n", cur-start, dev->id, dev->addr, devmodename[dev->devmode], devmodename[mode]);
    dev->devmode = mode;
    dev->serial++;
  }


void dev_input(struct device *dev, struct frame fr);

void dev_send(struct device *dev, int addr, int *data)
  {
    struct frame fr1;
    fr1.device_id = dev->port_dev[0];
    fr1.port_id = dev->port_port[0];
    fr1.src_addr = dev->addr;
    fr1.dst_addr = addr;
    fr1.ttl = 0;
    memcpy(fr1.data, data, sizeof(fr1.data));
    ether_put(fr1);
  }

void dev_new_getmaster(struct device *dev)
  {
    int dt[PKTLOAD];
    dt[0]=PKT_GETMASTER;
    dt[1]=dev->prio;
    dev_send(dev,-1,dt);
  }


void dev_master_ready(struct device * dev)
  {
    int dt[PKTLOAD];
    dt[0] = PKT_MASTER; // "master_ready"
    dt[1] = dev->prio;
    dt[2] = dev->master_color;
    dev_send(dev, -1, dt);
    jobs_put(cur + 20, &dev_master_ready, dev);
  }

void dev_master_main(struct device * dev)
  {
// TODO get&calc
//        struct frame fr;
//        fr.data[0] = PKT_MASTER; // "master_ready"
//        dev_send(dev, -1, fr.data);
    int fail = 0;
    int tempsum = 0;
    int lightsum = 0;
    int weightsum = 0;
    dev->main_jobs++;

    int dt[PKTLOAD];
    dt[0]=PKT_GET;
    dt[1]=dev->prio;

    for(int i=0; i < dev->child_num; i++)
      {
        if(cur - dev->child[i].last > 10)
          {
            fail++;
            dev->child[i].weight /= 2;
          }
        tempsum += dev->child[i].temp*dev->child[i].weight;
        lightsum += dev->child[i].light*dev->child[i].weight;
        weightsum += dev->child[i].weight;

        dev_send(dev, dev->child[i].addr, dt);
      }

    dev->child_fail = fail;

    if(weightsum >0)
      {
        tempsum /= weightsum;
        lightsum /= weightsum;

        dev->disp_temp = tempsum;
        dev->disp_light = lightsum;

        dt[0]=PKT_SET;
        dt[1]=dev->prio;
        dt[2]=tempsum;
        dt[3]=lightsum;

        for(int i=0; i < dev->child_num; i++)
          {
            dev_send(dev, dev->child[i].addr, dt);
          }
      }
    jobs_put(cur + 5, &dev_master_main, dev);
  }


void dev_elect_start(struct device *dev);


void dev_client_check(struct device * dev)
  {
    if(dev->master_last+2*CLIENTCHECK < cur)
      {
// //        printf("%d : change %d device lost connect to master %d\n", cur-start, dev->id, dev->master_addr);
        dev_elect_start(dev);
      }

    jobs_put(cur + CLIENTCHECK, &dev_client_check, dev);
  }

void devmode_client(struct device *dev)
  {
    devmode_set(dev, DEVMODE_CLIENT);
    jobs_put(cur + CLIENTCHECK, &dev_client_check, dev);
  }

int global_master_color=1;

void devmode_master(struct device *dev)
  {
    devmode_set(dev, DEVMODE_MASTER);
    dev->main_jobs = 0;
    dev->master_color = global_master_color++;
    if(global_master_color > 6) global_master_color=1;
    jobs_put(cur+1, &dev_master_ready, dev);
//    dev_master_ready(dev);
    dev_master_main(dev);
    sprintf(log_buff, "The device %d became the master", dev->id);
    log_print(log_buff);
  }

void dev_elect_finish(struct device *dev)
  {
    if(dev->master_addr) devmode_client(dev);
    else devmode_master(dev);
  }

void devmode_elect(struct device *dev)
  {
    devmode_set(dev, DEVMODE_ELECT);
    dev->master_color = 0;
    dev->master_addr = 0; // self
    dev->master_prio = dev->prio*10000000+dev->addr;
    dev->child_num = 0;
    jobs_put(cur + 5, &dev_elect_finish, dev);
  }

void dev_elect_start(struct device *dev)
  {
    int dt[PKTLOAD];
    dt[0]=PKT_ELECT_START;
    dt[1]=dev->prio;
    dev_send(dev,-1,dt);
    devmode_elect(dev);
    sprintf(log_buff, "The device %d starts the election process", dev->id);
    log_print(log_buff);
  }


void devmode_new(struct device *dev)
  {
    devmode_set(dev, DEVMODE_NEW);
    dev->master_last = cur;
    dev_new_getmaster(dev);
    jobs_put(cur+5, &dev_new_getmaster, dev);
    jobs_put(cur+10, &dev_elect_start, dev);
  }

void dev_init(int d, int sw, int swp)
  {
    nodes[d].id=d;
    nodes[d].type = 'D';
    nodes[d].input = dev_input;
    nodes[d].swfail = 0;
    nodes[d].port_dev[0]=sw;
    nodes[d].port_port[0]=swp;
    nodes[d].addr=1000000+sw*1000+d;
    nodes[sw].port_dev[swp]=d;
    nodes[sw].port_port[swp]=0;
    nodes[sw].child_num=0;
    devmode_new(nodes+d);
  }

void dev_input(struct device *dev, struct frame fr)
  {
//    dev_print(dev);
    if( (fr.data[0] >0) &&(fr.data[0] <= PKT_LAST) )
      {
// //        if( (dev->id == 102) || (dev->id == 220)) 
// //            printf("%d : device %d Recived from %d => %d (%s) %d\n", cur-start, dev->id, fr.src_addr, fr.dst_addr, pktname[fr.data[0]], fr.data[1]);
      }
    else
      {
//        printf("device %d Recived from %d => %d (%d) -- skip\n", dev->id, fr.src_addr, fr.dst_addr, fr.data[0]);
        return;
      }
    if( (fr.dst_addr != -1) && (fr.dst_addr != dev->addr) )
      {
// //        printf("Wrong dst!\n");
        return;
      }

    if(fr.data[0] == PKT_PING) // "ping"
      {
        fr.data[0]=PKT_PONG; // "pong"
        dev_send(dev, fr.src_addr, fr.data);
        return;
      }

    if(fr.data[0] == PKT_ELECT_START) // "elect_start"
      {
        devmode_elect(dev);
        dev_addchild(dev, fr.src_addr, fr.data[1]*10000000+fr.src_addr);
        fr.data[0]=PKT_ELECT; // "elect"
        fr.data[1]=dev->prio;
        dev_send(dev, -1, fr.data);
        return;
      }


    if(fr.data[0] == PKT_ELECT) // "elect"
      {
        if(dev->devmode == DEVMODE_ELECT)
          {
            dev_addchild(dev, fr.src_addr, fr.data[1]*10000000+fr.src_addr);
            return;
          }
        if(dev->devmode == DEVMODE_MASTER)
          {
            if( dev->master_prio > fr.data[1]*10000000+fr.src_addr )
              {
                dev_addchild(dev, fr.src_addr, fr.data[1]*10000000+fr.src_addr);
              }
            else
              {
                dev_elect_start(dev);
              }
            return;
          }
        return;
      }

    if(fr.data[0] == PKT_GETMASTER) // "getmaster"
      {
        if(dev->devmode == DEVMODE_MASTER)
          {
            fr.data[0]=PKT_MASTER; // "master_ready"
            fr.data[1]=dev->prio;
            fr.data[2]=dev->master_color;
            dev_send(dev, fr.src_addr, fr.data);
          }
        return;
      }

    if(fr.data[0] == PKT_MASTER) // "master_ready"
      {
        if(dev->devmode == DEVMODE_MASTER) //2 masters
          {
            dev_elect_start(dev);
            return;
          }
        if(dev->devmode == DEVMODE_NEW)
          {
            if( (fr.data[1]*10000000+fr.src_addr) > (dev->prio*10000000+dev->addr) )
              {
                dev->master_addr = fr.src_addr;
                dev->master_prio = fr.data[1]*10000000+fr.src_addr;
                dev->master_color = fr.data[2];
                devmode_client(dev);
                dev->master_last=cur;
                fr.data[0]=PKT_ELECT; // "elect"
                fr.data[1]=dev->prio;
                dev_send(dev, -1, fr.data);
              }
            else
              {
                dev_elect_start(dev);
              }
            return;
          }
        if(dev->devmode == DEVMODE_CLIENT)
          {
            dev->master_last=cur;
            dev->master_color = fr.data[2];
            return;
          }
        return;
      }
    if(fr.data[0] == PKT_GET)
      {
        if(dev->devmode == DEVMODE_CLIENT)
          {
            int tm = (cur + 5*dev->id) % 1000;
            int temp = 1000 + 3*(tm >500? 1000-tm : tm); // 1000 .. 2500  (10.00 .. 25.00)
            int light = 100 + (tm >500? 1000-tm : tm); // 100 .. 600
            fr.data[0] = PKT_RPL;
            fr.data[2] = temp;
            fr.data[3] = light;
            dev_send(dev, fr.src_addr, fr.data);
          }
        return;
      }

    if(fr.data[0] == PKT_SET)
      {
        if(dev->devmode == DEVMODE_CLIENT)
          {
            dev->disp_temp = fr.data[2];
            dev->disp_light = fr.data[3];
          }
        return;
      }

    if(fr.data[0] == PKT_RPL)
      {
        if(dev->devmode == DEVMODE_MASTER)
          {
            int temp = fr.data[2];
            int light = fr.data[3];
            dev_updatechild(dev, fr.src_addr, light, temp);
          }
        return;
      }



  }


void drop_uplink(struct device *dev)
  {
    dev->portfail[0]=cur+1000000;
    nodes[dev->port_dev[0]].portfail[dev->port_port[0]]=cur+10000;
    sprintf(log_buff, "Drop link from dev %d to dev %d", dev->id, dev->port_dev[0]);
    log_print(log_buff);
  }

void up_uplink(struct device *dev)
  {
    dev->portfail[0]=0;
    nodes[dev->port_dev[0]].portfail[dev->port_port[0]]=0;
    sprintf(log_buff, "Up link from dev %d to dev %d", dev->id, dev->port_dev[0]);
    log_print(log_buff);
  }

void add_clnt(struct device *dev)
  {
    dev_init(231, 3, 7);
    sprintf(log_buff, "Add new dev with lo prio");
    log_print(log_buff);
  }

void add_mast(struct device *dev)
  {
    dev_init(232, 64, 7);
    sprintf(log_buff, "Add new dev with HI PRIO");
    log_print(log_buff);
  }



int main()
  {
    setvbuf(stdout, 0, _IONBF, 0);
    printf("\033[2J");

    start = cur = time(0);
    char str[999];
    FILE * file;
    file = fopen( "map.txt" , "r");
    int n,d,p;
    int endpoint=100;
    if (file)
      {
        while (fscanf(file, "%s %d %d %d", str,&n,&d,&p)!=EOF)
          {
            nodes[n].id = n;
            nodes[n].type = 'S';
            nodes[n].input = sw_input;
            nodes[n].swfail = 0;
            nodes[n].packets = 0;
            nodes[n].bpackets = 0;
            for( int j=0; j<MAXNODES; j++)
              {
                nodes[n].addrtbl[j].addr = 0;
                nodes[n].addrtbl[j].port = 0;
              }
            for( int i=0; i<MAXPORTS; i++)
              {
                nodes[n].portfail[i] = 0;
                nodes[n].port_dev[i] = -1;
                nodes[n].port_port[i] = -1;
              }
            if( d != -1 )
              {
                nodes[n].port_dev[0]=d;
                nodes[n].port_port[0]=p;
                nodes[d].port_dev[p]=n;
                nodes[d].port_port[p]=0;
              }

//            if(n<12)
              {
                dev_init(endpoint++, n, 5);
                dev_init(endpoint++, n, 6);
              }

//            printf("%s/%d/%d/%d\n",str,n,d,p);
              dev_print(nodes+n);
          }
        fclose(file);
//        exit(0);
      }
    else exit(10);

    int zzz[PKTLOAD];
    zzz[0]=PKT_PING;

    jobs_put(cur+40, &add_clnt, nodes);
    jobs_put(cur+80, &add_mast, nodes);
    jobs_put(cur+120, &drop_uplink, nodes+1);
    jobs_put(cur+240, &up_uplink, nodes+1);





//    dev_send(nodes+100,-1,zzz);
//    dev_send(nodes+110,-1,zzz);
    runq();

//    dev_send(nodes+100,2060220,zzz);
//    dev_send(nodes+220,2000100,zzz);
//    runq();


    printf("%d %d %d\n",cur, ether_head, ether_tail);
  }




