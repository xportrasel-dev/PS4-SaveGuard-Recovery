#include <sqlite3.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef int (*sg_db_cb)(const char *,const char *,const char *,int64_t,void *);
int sg_db_inventory(const char *path,sg_db_cb cb,void *ctx){
 sqlite3 *db=NULL;sqlite3_stmt *st=NULL;int count=0,rc;
 if(!path||!cb)return -1;
 rc=sqlite3_open_v2(path,&db,SQLITE_OPEN_READONLY|SQLITE_OPEN_PRIVATECACHE,NULL);
 if(rc!=SQLITE_OK){sqlite3_close(db);return -1;}
 if(sqlite3_exec(db,"PRAGMA query_only=ON; PRAGMA temp_store=MEMORY;",NULL,NULL,NULL)!=SQLITE_OK)goto fail;
 if(sqlite3_prepare_v2(db,"SELECT title_id, dir_name, main_title, blocks FROM savedata",-1,&st,NULL)!=SQLITE_OK)goto fail;
 while((rc=sqlite3_step(st))==SQLITE_ROW){
 const char *title=(const char *)sqlite3_column_text(st,0),*dir=(const char *)sqlite3_column_text(st,1),*name=(const char *)sqlite3_column_text(st,2);
 if(!title||!dir||!name||count>=65536||cb(title,dir,name,sqlite3_column_int64(st,3),ctx))goto fail;
 count++;
 }
 sqlite3_finalize(st);sqlite3_close(db);return rc==SQLITE_DONE?count:-1;
 fail:sqlite3_finalize(st);sqlite3_close(db);return -1;
}
int sg_db_user_inventory(uint32_t user,sg_db_cb cb,void *ctx){
 char p[128];snprintf(p,sizeof p,"/system_data/savedata/%08x/db/user/savedata.db",(unsigned)user);
 return sg_db_inventory(p,cb,ctx);
}

#include <sys/stat.h>
#include <errno.h>
struct sg_db_summary {unsigned rows,present,missing,unknown;};
struct sg_db_context {uint32_t user;struct sg_db_summary *s;};
static int classify(const char *title,const char *dir,const char *name,int64_t blocks,void *ctx){
 (void)dir;(void)name;(void)blocks;
 struct sg_db_context *c=ctx;struct sg_db_summary *s=c->s; s->rows++;
 if(strlen(title)!=9||strncmp(title,"CUSA",4)||strspn(title+4,"0123456789")!=5){s->unknown++;return 0;}
 char root[128],p[160];struct stat st;
 snprintf(root,sizeof root,"/user/home/%08x/savedata",(unsigned)c->user);
 if(lstat(root,&st)||!S_ISDIR(st.st_mode)){s->unknown++;return 0;}
 snprintf(p,sizeof p,"%s/%s",root,title);
 if(lstat(p,&st)){if(errno==ENOENT)s->missing++;else s->unknown++;}
 else if(S_ISDIR(st.st_mode))s->present++;else s->unknown++;
 return 0;
}
int sg_db_reconcile(uint32_t user,struct sg_db_summary *s){
 if(!s)return -1;
 memset(s,0,sizeof *s);
 struct sg_db_context c={user,s};
 return sg_db_user_inventory(user,classify,&c);
}

#include <dirent.h>
#include <stdlib.h>
int sg_db_scan_all(struct sg_db_summary *total,unsigned *users,unsigned *failed){
 if(!total||!users||!failed)return -1;
 memset(total,0,sizeof *total);*users=*failed=0;
 DIR *d=opendir("/system_data/savedata");if(!d)return -1;
 struct dirent *e;int rc=0;
 for(;;){
 errno=0;e=readdir(d);if(!e){if(errno)rc=-1;break;}
 if(strlen(e->d_name)!=8||strspn(e->d_name,"0123456789abcdefABCDEF")!=8)continue;
 if(*users>=256){rc=-1;break;}
 ++*users;
 struct sg_db_summary s;
 if(sg_db_reconcile((uint32_t)strtoul(e->d_name,NULL,16),&s)<0){++*failed;rc=-1;continue;}
 total->rows+=s.rows;total->present+=s.present;total->missing+=s.missing;total->unknown+=s.unknown;
 }
 closedir(d);return *users?rc:-1;
}
