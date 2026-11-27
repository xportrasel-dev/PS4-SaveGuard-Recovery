#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
typedef void (*sg_history_cb)(const char *,const char *,const char *,long long,void *);
int sg_history(unsigned usb,sg_history_cb cb,void *ctx){
 if(usb>7||!cb)return -1;
 char root[64];snprintf(root,sizeof root,"/mnt/usb%u",usb);
 DIR *d=opendir(root);if(!d)return -1;
 struct dirent *e;int count=0;
 while((e=readdir(d))){
 char u[9]={0},t[10]={0},p[512],m[600];long long stamp;int n=0;
 if(sscanf(e->d_name,"SaveGuard-%8[0123456789abcdefABCDEF]-%9[CUSA0123456789]-%lld%n",u,t,&stamp,&n)!=3||e->d_name[n]||strlen(u)!=8||strlen(t)!=9||strncmp(t,"CUSA",4))continue;
 int valid=1;for(int i=4;i<9;i++)if(t[i]<'0'||t[i]>'9')valid=0;
 if(!valid)continue;
 struct stat st;
 snprintf(p,sizeof p,"%s/%s",root,e->d_name);
 if(lstat(p,&st)||!S_ISDIR(st.st_mode))continue;
 snprintf(m,sizeof m,"%s/.complete",p);
 if(lstat(m,&st)||!S_ISREG(st.st_mode))continue;
 snprintf(m,sizeof m,"%s/data",p);
 if(lstat(m,&st)||!S_ISDIR(st.st_mode))continue;
 snprintf(m,sizeof m,"%s/manifest.txt",p);
 if(lstat(m,&st)||!S_ISREG(st.st_mode))continue;
 cb(p,u,t,stamp,ctx);count++;
 }
 closedir(d);return count;
}
