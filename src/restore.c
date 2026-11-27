int sg_usb_ready(unsigned);
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
int sg_verify_snapshot(const char *,unsigned *);
int sg_export(const char *,const char *);
static int stage_tree(const char *src,const char *dst,unsigned depth){
 if(!depth)return -1;
 DIR *d=opendir(src);if(!d)return -1;
 int rc=0;struct dirent *e;
 for(;;){
 errno=0;e=readdir(d);if(!e){if(errno)rc=-1;break;}
 if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
 char a[4096],b[4096];struct stat st;
 int x=snprintf(a,sizeof a,"%s/%s",src,e->d_name),y=snprintf(b,sizeof b,"%s/%s",dst,e->d_name);
 if(x<0||y<0||x>=4096||y>=4096||lstat(a,&st)){rc=-1;break;}
 if(S_ISDIR(st.st_mode)){if(mkdir(b,0700)||stage_tree(a,b,depth-1)){rc=-1;break;}}
 else if(!S_ISREG(st.st_mode)||sg_export(a,b)){rc=-1;break;}
 }
 closedir(d);return rc;
}
/* Stage raw files beside the snapshot. Never overwrite active console saves. */
int sg_stage_restore(const char *root){
 unsigned checked;char dst[4096],src[4096],out[4096];struct stat st;
 if(!root||strlen(root)>3800||sg_verify_snapshot(root,&checked))return -1;
 if(strncmp(root,"/mnt/usb",8)||root[8]<'0'||root[8]>'7'||strncmp(root+9,"/SaveGuard-",11)||strchr(root+10,'/'))return -1;
 if(sg_usb_ready((unsigned)(root[8]-'0')))return -1;
 snprintf(dst,sizeof dst,"%s-recovery-%lld",root,(long long)time(NULL));
 if(mkdir(dst,0700))return -1;
 snprintf(src,sizeof src,"%s/data",root);
 snprintf(out,sizeof out,"%s/data",dst);
 if(lstat(src,&st)||!S_ISDIR(st.st_mode)||mkdir(out,0700)||stage_tree(src,out,16))return -1;
 const char *names[]={"manifest.txt","checksums.sha256",".complete"};
 for(unsigned i=0;i<3;i++){
 snprintf(src,sizeof src,"%s/%s",root,names[i]);
 snprintf(out,sizeof out,"%s/%s",dst,names[i]);
 if(sg_export(src,out))return -1;
 }
 return sg_verify_snapshot(dst,&checked);
}
