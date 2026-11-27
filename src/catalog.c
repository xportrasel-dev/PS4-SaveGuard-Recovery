#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
typedef void (*sg_catalog_cb)(const char *,const char *,const char *,void *);
static int title_id(const char *s){
 if(strlen(s)!=9||memcmp(s,"CUSA",4))return 0;
 for(int i=4;i<9;i++)if(s[i]<'0'||s[i]>'9')return 0;
 return 1;
}
static int user_id(const char *s){
 if(strlen(s)!=8)return 0;
 for(int i=0;i<8;i++)if(!isxdigit((unsigned char)s[i]))return 0;
 return 1;
}
int sg_catalog(sg_catalog_cb cb,void *ctx){
 if(!cb)return -1;
 DIR *users=opendir("/user/home");if(!users)return -1;
 struct dirent *u;int count=0,partial=0;
 while((u=readdir(users))){
 if(!user_id(u->d_name))continue;
 char root[128];struct stat st;
 snprintf(root,sizeof root,"/user/home/%.8s/savedata",u->d_name);
 if(lstat(root,&st)||!S_ISDIR(st.st_mode)){partial=1;continue;}
 DIR *games=opendir(root);if(!games){partial=1;continue;}
 struct dirent *g;
 while((g=readdir(games))){
 if(!title_id(g->d_name))continue;
 char path[256];snprintf(path,sizeof path,"%s/%.9s",root,g->d_name);
 if(lstat(path,&st)||!S_ISDIR(st.st_mode)){partial=1;continue;}
 cb(u->d_name,g->d_name,path,ctx);count++;
 }
 closedir(games);
 }
 closedir(users);return partial?-1:count;
}
