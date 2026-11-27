#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
int sg_verify_snapshot(const char *,unsigned *);
int sg_trash_move(unsigned usb,const char *name,int restore){
 if(usb>7||!name||strlen(name)>100)return -1;
 char u[9]={0},t[10]={0};long long stamp;int n=0;
 if(sscanf(name,"SaveGuard-%8[0123456789abcdefABCDEF]-%9[CUSA0123456789]-%lld%n",u,t,&stamp,&n)!=3||name[n]||strlen(u)!=8||strlen(t)!=9||strncmp(t,"CUSA",4))return -1;
 for(int i=4;i<9;i++)if(t[i]<'0'||t[i]>'9')return -1;
 char root[64],trash[128],a[256],b[256];struct stat st;
 snprintf(root,sizeof root,"/mnt/usb%u",usb);
 if(lstat(root,&st)||!S_ISDIR(st.st_mode))return -1;
 snprintf(trash,sizeof trash,"%s/SaveGuard-Trash",root);
 if(!restore&&mkdir(trash,0700)&&errno!=EEXIST)return -1;
 if(lstat(trash,&st)||!S_ISDIR(st.st_mode))return -1;
 snprintf(a,sizeof a,"%s/%s",restore?trash:root,name);
 snprintf(b,sizeof b,"%s/%s",restore?root:trash,name);
 if(lstat(a,&st)||!S_ISDIR(st.st_mode)||sg_verify_snapshot(a,NULL))return -1;
 if(lstat(b,&st)==0||errno!=ENOENT)return -1;
 /* Single-process prototype; concurrent external writers are unsupported. */
 return rename(a,b)?-1:0;
}
