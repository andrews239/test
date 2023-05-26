

char * colors[] = { "\033[37;40m", "\033[37;41m","\033[30;42m","\033[30;43m","\033[37;44m","\033[37;45m","\033[37;46m" };




void dev_print(struct device *dev)
  {
    int id = dev->id;
    int X = 1+6*(id % 30);
    int Y = 1+4*(id / 30);
    char * color = colors[dev->master_color];
    printf("\033[%d;%dH\033[Ktime: %d",34,1,cur-start);
    printf("%s", color);
    printf("\033[%d;%dH,----.",Y,X);
    if(dev->type=='S')
      {
        printf("\033[%d;%dH,----.",Y,X);
        if(dev->packets >9999) printf("\033[%d;%dH|%3dk|",Y+1,X,dev->packets/1024);
        else printf("\033[%d;%dH|%4d|",Y+1,X,dev->packets);
        if(dev->bpackets >999) printf("\033[%d;%dH|B%2dk|",Y+2,X,dev->bpackets/1024);
        else printf("\033[%d;%dH|B%3d|",Y+2,X,dev->bpackets);
        printf("\033[%d;%dH`----'",Y+3,X);
      }
    else
      {
        if(dev->devmode==DEVMODE_MASTER)
          {
            printf("\033[%d;%dH######",Y,X);
            printf("\033[%d;%dH#%4.1f#",Y+1,X,dev->disp_temp/100.0);
            printf("\033[%d;%dH#%4.1f#",Y+2,X,dev->disp_light/100.0);
            printf("\033[%d;%dH######",Y+3,X);


            printf("\033[%d;%dH\033[K",35+dev->master_color,1);
            printf("%d: Addr: %d Clients: %d (%d -- fail) jobs: %d temp: %.1f, light: %.1f",cur-start, dev->addr, dev->child_num+1, dev->child_fail, dev->main_jobs, dev->disp_temp/100.0, dev->disp_light/100.0);

          }
        else
          {
            printf("\033[%d;%dH,----.",Y,X);
            printf("\033[%d;%dH|%4.1f|",Y+1,X,dev->disp_temp/100.0);
            printf("\033[%d;%dH|%4.1f|",Y+2,X,dev->disp_light/100.0);
            printf("\033[%d;%dH`----'",Y+3,X);
          }
      }
    printf("%s","\033[m");
  }


int log_str=45;
char log_buff[1000];
void log_print(char * str)
  {
    printf("\033[%d;%dH\033[K%d:%s\n\033[K",log_str++,1,cur-start,str);
    if(log_str>57) log_str=45;
  }
