time_t start;
time_t cur;
struct job jobs[MAXNODES*5];
int job_num=0;

int jobs_ready()
  {
    if(job_num) return 1;
    return 0;
  }

struct job jobs_get()
  {
    int first=INT_MAX;
    int first_id=-1;
    int jobs_n=job_num;
    struct job res;
    for(int i=0; jobs_n; i++)
      {
        if(jobs[i].start_at)
          {
            jobs_n--;
            if(jobs[i].start_at < first)
              {
                first=jobs[i].start_at;
                first_id=i;
              }
          }
      }
    if(first > cur)
      {
        sleep(first - cur);
        cur = time(0);
      }
    res = jobs[first_id];
    jobs[first_id].start_at=0;
    job_num--;
    return res;
  }

void jobs_put(int start_at, void (*call)(struct device *), struct device * param) {
    for(int i=0;; i++)
      {
        if(!jobs[i].start_at)
          {
            jobs[i].start_at = start_at;
            jobs[i].call = call;
            jobs[i].param = param;
            jobs[i].serial = param->serial;
            jobs[i].devmode = param->devmode;
            job_num++;
            break;
          }
      }
}


