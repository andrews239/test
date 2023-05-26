void resend_frame(struct device dev, int port, int src, struct frame fr)
  {
//    printf("resend: %d -> %d\n", src, port);
    if(port == src) return;
    if(cur < dev.portfail[port]) return;
    if(dev.port_dev[port] == -1) return;

    fr.device_id = dev.port_dev[port];
    fr.port_id = dev.port_port[port];
    fr.ttl++;
    ether_put(fr);
  }


void sw_input(struct device *dev, struct frame fr)
  {
    int src_port = fr.port_id;
    if(cur < dev->swfail) return;

    dev->packets++;

//    printf("sw_input S SW%d mac:%d pack:%d->%d\n",dev->id, dev->addrtbl[0].addr, fr.src_addr,fr.dst_addr);

    for( int j=0; j<MAXNODES; j++)
      {
        if(dev->addrtbl[j].addr == fr.src_addr)
          {
            if(dev->addrtbl[j].port == fr.port_id) break;
            dev->addrtbl[j].port = fr.port_id;
            break;
          }
        if(dev->addrtbl[j].addr == 0)
          {
//            printf("add %d to port %d.%d N%d\n", fr.src_addr, dev->id, fr.port_id,j);
            dev->addrtbl[j].addr = fr.src_addr;
            dev->addrtbl[j].port = fr.port_id;
            break;
          }
      }
    int dst_port=-1;
    if(fr.dst_addr != -1)
      {
//        printf("SW %d:",dev->id);
        for( int j=0; (dst_port==-1) && (j<MAXNODES) && (dev->addrtbl[j].addr != 0); j++)
          {
//            printf("%d-%d|",dev->addrtbl[j].addr,dev->addrtbl[j].port);
            if(dev->addrtbl[j].addr == fr.dst_addr)
              {
                dst_port = dev->addrtbl[j].port;
//                printf("!!!");
              }
          }
//        printf("\n");
      }
    if(dst_port == -1)
      {
        dev->bpackets++;
        for( int i=0; i<MAXPORTS; i++)
            resend_frame(*dev, i, src_port, fr);
      }
    else
        resend_frame(*dev, dst_port, src_port, fr);

//    printf("sw_input E SW%d mac:%d\n",dev->id, dev->addrtbl[0].addr);
  };
