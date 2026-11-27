#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
struct sg_schedule {uint64_t interval_seconds,next_due;unsigned enabled,usb,running;};
int sg_schedule_configure(struct sg_schedule *,unsigned,unsigned,uint64_t,uint64_t);
static int settings_path(unsigned usb,char p[128]){
 if(usb>7)return -1;
 char root[64];struct stat st;
 snprintf(root,sizeof root,"/mnt/usb%u",usb);
 if(lstat(root,&st)||!S_ISDIR(st.st_mode))return -1;
 snprintf(p,128,"%s/SaveGuard-settings.txt",root);return 0;
}
int sg_settings_save(const struct sg_schedule *s){
 char p[128],tmp[160];if(!s||settings_path(s->usb,p))return -1;
 snprintf(tmp,sizeof tmp,"%s.tmp",p);
 int fd=open(tmp,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd<0)return -1;
 FILE *f=fdopen(fd,"w");if(!f){close(fd);unlink(tmp);return -1;}
 int rc=fprintf(f,"SG1 %u %llu\n",!!s->enabled,(unsigned long long)s->interval_seconds)<0;
 if(fflush(f)||fsync(fd))rc=1;
 if(fclose(f))rc=1;
 if(!rc&&rename(tmp,p)==0)return 0;
 unlink(tmp);return -1;
}
int sg_settings_load(struct sg_schedule *s,unsigned usb,uint64_t now){
 char p[128],line[128],extra;unsigned enabled;unsigned long long interval;
 if(!s||settings_path(usb,p))return -1;
 int fd=open(p,O_RDONLY|O_NOFOLLOW);if(fd<0)return -1;
 struct stat st;if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size>=128){close(fd);return -1;}
 FILE *f=fdopen(fd,"r");if(!f){close(fd);return -1;}
 int ok=fgets(line,sizeof line,f)&&sscanf(line,"SG1 %u %llu %c",&enabled,&interval,&extra)==2&&enabled<=1;
 fclose(f);
 if(!ok)return -1;
 /* Loading settings never arms unattended backups. */
 return sg_schedule_configure(s,0,usb,(uint64_t)interval,now);
}
