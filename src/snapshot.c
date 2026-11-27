#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
int sg_export(const char *,const char *);
static int copy_tree(const char *src,const char *dst,unsigned depth){
 if(!depth)return -1;
 DIR *d=opendir(src); if(!d)return -1;
 int rc=0; struct dirent *e;
 while((e=readdir(d))){
 if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
 char a[4096],b[4096]; struct stat st;
 int x=snprintf(a,sizeof a,"%s/%s",src,e->d_name);
 int y=snprintf(b,sizeof b,"%s/%s",dst,e->d_name);
 if(x<0||y<0||(size_t)x>=sizeof a||(size_t)y>=sizeof b||lstat(a,&st)){rc=-1;break;}
 if(S_ISDIR(st.st_mode)){if(mkdir(b,0700)||copy_tree(a,b,depth-1)){rc=-1;break;}}
 else if(S_ISREG(st.st_mode)){if(sg_export(a,b)){rc=-1;break;}}
 else {rc=-1;break;}
 }
 closedir(d);return rc;
}
/* Raw file snapshot, not a mount-aware or guaranteed restorable PS4 save. */
int sg_snapshot(const char *src,unsigned usb){
 if(usb>7)return -1;
 char root[128],dst[256],marker[300],data[300],meta[300],user[9]={0},title[10]={0}; struct stat st;
 int consumed=0;
 if(sscanf(src,"/user/home/%8[0123456789abcdefABCDEF]/savedata/%9[CUSA0123456789]%n",user,title,&consumed)!=2 || src[consumed] || strlen(user)!=8 || strlen(title)!=9 || strncmp(title,"CUSA",4))return -1;
 for(int i=4;i<9;i++)if(title[i]<'0'||title[i]>'9')return -1;
 snprintf(root,sizeof root,"/mnt/usb%u",usb);
 if(stat(root,&st)||!S_ISDIR(st.st_mode))return -1;
 long long stamp=(long long)time(NULL);
 snprintf(dst,sizeof dst,"%s/SaveGuard-%s-%s-%lld",root,user,title,stamp);
 if(mkdir(dst,0700))return -1;
 snprintf(data,sizeof data,"%s/data",dst);
 if(mkdir(data,0700)||copy_tree(src,data,16))return -1;
 snprintf(meta,sizeof meta,"%s/manifest.txt",dst);
 FILE *m=fopen(meta,"wx");if(!m)return -1;
 int written=fprintf(m,"format=1\nuser=%s\ntitle=%s\ncreated=%lld\nkind=raw-files\n",user,title,stamp);
 int closed=fclose(m);if(written<0||closed)return -1;
 snprintf(marker,sizeof marker,"%s/.complete",dst);
 FILE *f=fopen(marker,"wx");if(!f)return -1;
 if(fputs("Verified file copies; restore compatibility untested.\n",f)<0){fclose(f);unlink(marker);return -1;}
 if(fclose(f)){unlink(marker);return -1;}return 0;
}
