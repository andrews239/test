struct frame ether[MAXETHER];
int ether_head=0;
int ether_tail=0;

int ether_ready()
  {
    if( ether_head == ether_tail ) return 0;
    return 1;
  }

struct frame ether_get()
  {
//    printf("GET %d -> %d => %d:%d\n",ether[ether_head].src_addr, ether[ether_head].dst_addr, ether[ether_head].device_id, ether[ether_head].port_id);
    if( ether_head == MAXETHER ) ether_head=0;
    if( ether_tail == MAXETHER ) ether_tail=0;
    return ether[ether_head++];
  }

void ether_put(struct frame fr)
  {
    char *sp="                        ";
//    printf("NEW pkt %d -> %d (ttl %d) %s => %d:%d\n", fr.src_addr, fr.dst_addr, fr.ttl, sp+(24-2*fr.ttl), fr.device_id, fr.port_id);
    if( ether_head == MAXETHER ) ether_head=0;
    if( ether_tail == MAXETHER ) ether_tail=0;
    if( (ether_head == ether_tail+1) || ((ether_head == 0) && (ether_tail == MAXETHER-1)) )
      {
        return; // overflow
      }
    ether[ether_tail++] = fr;
  }

