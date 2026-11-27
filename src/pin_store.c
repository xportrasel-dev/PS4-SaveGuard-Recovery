int sg_usb_ready(unsigned);
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
struct sg_pin_record {unsigned char salt[16],hash[32];};
static int path_for(unsigned usb,char p[128]){
 if(sg_usb_ready(usb))return -1;
 char root[64];struct stat st;
 snprintf(root,sizeof root,"/mnt/usb%u",usb);
 if(lstat(root,&st)||!S_ISDIR(st.st_mode))return -1;
 snprintf(p,128,"%s/SaveGuard-pin.bin",root);return 0;
}
int sg_pin_store_new(unsigned usb,const struct sg_pin_record *r){
 char p[128];unsigned char b[52];
 if(!r||path_for(usb,p))return -1;
 memcpy(b,"SGP1",4);memcpy(b+4,r->salt,16);memcpy(b+20,r->hash,32);
 int fd=open(p,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd<0)return -1;
 size_t off=0;
 while(off<sizeof b){ssize_t n=write(fd,b+off,sizeof b-off);if(n<0&&errno==EINTR)continue;if(n<=0)break;off+=(size_t)n;}
 int rc=off!=sizeof b||fsync(fd);if(close(fd))rc=1;
 if(rc){unlink(p);return -1;}return 0;
}
int sg_pin_load(unsigned usb,struct sg_pin_record *r){
 char p[128];unsigned char b[52];struct stat st;
 if(!r||path_for(usb,p))return -1;
 int fd=open(p,O_RDONLY|O_NOFOLLOW);if(fd<0)return -1;
 if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size!=sizeof b){close(fd);return -1;}
 size_t off=0;
 while(off<sizeof b){ssize_t n=read(fd,b+off,sizeof b-off);if(n<0&&errno==EINTR)continue;if(n<=0)break;off+=(size_t)n;}
 close(fd);
 if(off!=sizeof b||memcmp(b,"SGP1",4))return -1;
 memcpy(r->salt,b+4,16);memcpy(r->hash,b+20,32);return 0;
}

int sg_pin_replace(unsigned usb,const struct sg_pin_record *old,const struct sg_pin_record *next){
 char p[128],tmp[160];struct sg_pin_record current;unsigned char b[52];
 if(!old||!next||path_for(usb,p)||sg_pin_load(usb,&current)||memcmp(&current,old,sizeof current))return -1;
 snprintf(tmp,sizeof tmp,"%s.new",p);
 memcpy(b,"SGP1",4);memcpy(b+4,next->salt,16);memcpy(b+20,next->hash,32);
 int fd=open(tmp,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd<0)return -1;
 size_t off=0;
 while(off<sizeof b){ssize_t n=write(fd,b+off,sizeof b-off);if(n<0&&errno==EINTR)continue;if(n<=0)break;off+=(size_t)n;}
 int rc=off!=sizeof b||fsync(fd);if(close(fd))rc=1;
 if(!rc)rc=sg_usb_ready(usb)||sg_pin_load(usb,&current)||memcmp(&current,old,sizeof current)||rename(tmp,p);
 if(rc){unlink(tmp);return -1;}return 0;
}
