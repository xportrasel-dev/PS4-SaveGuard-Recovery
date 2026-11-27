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

typedef void (*sg_orphan_cb)(uint32_t,const char *,void *);
int sg_orphan_titles(uint32_t user,sg_orphan_cb cb,void *ctx){
 char root[128],dbpath[128];sqlite3 *db=NULL;sqlite3_stmt *st=NULL;DIR *d=NULL;int count=0,rc=-1;
 if(!cb)return -1;
 snprintf(root,sizeof root,"/user/home/%08x/savedata",(unsigned)user);
 snprintf(dbpath,sizeof dbpath,"/system_data/savedata/%08x/db/user/savedata.db",(unsigned)user);
 if(sqlite3_open_v2(dbpath,&db,SQLITE_OPEN_READONLY|SQLITE_OPEN_PRIVATECACHE,NULL)!=SQLITE_OK)goto done;
 if(sqlite3_exec(db,"PRAGMA query_only=ON; PRAGMA temp_store=MEMORY;",NULL,NULL,NULL)!=SQLITE_OK)goto done;
 if(sqlite3_prepare_v2(db,"SELECT 1 FROM savedata WHERE title_id=? LIMIT 1",-1,&st,NULL)!=SQLITE_OK)goto done;
 d=opendir(root);if(!d)goto done;
 for(;;){
 errno=0;struct dirent *e=readdir(d);if(!e){if(!errno)rc=count;break;}
 if(strlen(e->d_name)!=9||strncmp(e->d_name,"CUSA",4)||strspn(e->d_name+4,"0123456789")!=5)continue;
 char path[160];struct stat sb;snprintf(path,sizeof path,"%s/%.9s",root,e->d_name);
 if(lstat(path,&sb)||!S_ISDIR(sb.st_mode))goto done;
 if(sqlite3_bind_text(st,1,e->d_name,-1,SQLITE_TRANSIENT)!=SQLITE_OK)goto done;
 int step=sqlite3_step(st);
 if(step==SQLITE_DONE){if(count>=65536)goto done;cb(user,e->d_name,ctx);count++;}
 else if(step!=SQLITE_ROW)goto done;
 if(sqlite3_reset(st)!=SQLITE_OK||sqlite3_clear_bindings(st)!=SQLITE_OK)goto done;
 }
 done:if(d)closedir(d);sqlite3_finalize(st);sqlite3_close(db);return rc;
}

int sg_orphan_scan_all(sg_orphan_cb cb,void *ctx,unsigned *found,unsigned *failed){
 if(!cb||!found||!failed)return -1;
 *found=*failed=0;
 DIR *d=opendir("/user/home");if(!d)return -1;
 unsigned users=0;int rc=0;
 for(;;){
 errno=0;struct dirent *e=readdir(d);if(!e){if(errno)rc=-1;break;}
 if(strlen(e->d_name)!=8||strspn(e->d_name,"0123456789abcdefABCDEF")!=8)continue;
 if(++users>256){rc=-1;break;}
 int n=sg_orphan_titles((uint32_t)strtoul(e->d_name,NULL,16),cb,ctx);
 if(n<0){++*failed;rc=-1;}else *found+=(unsigned)n;
 }
 closedir(d);return users?rc:-1;
}

/* 0 absent, 1 nonempty regular file, -1 inaccessible or invalid. */
static int pair_file(const char *p){
 struct stat st;
 if(lstat(p,&st))return errno==ENOENT?0:-1;
 return S_ISREG(st.st_mode)&&st.st_size>0?1:-1;
}
/* 1 complete candidate, 0 missing component, -1 unknown, 2 ambiguous volume. */
int sg_pair_check(const char *root,const char *slot){
 char key[1024],volume[1024],legacy[1024];
 if(!root||!slot||!*slot||strlen(slot)>255||strchr(slot,'/')||!strcmp(slot,".")||!strcmp(slot,".."))return -1;
 int a=snprintf(key,sizeof key,"%s/%s.bin",root,slot);
 int b=snprintf(volume,sizeof volume,"%s/sdimg_%s",root,slot);
 int c=snprintf(legacy,sizeof legacy,"%s/%s",root,slot);
 if(a<0||b<0||c<0||a>=(int)sizeof key||b>=(int)sizeof volume||c>=(int)sizeof legacy)return -1;
 struct stat st;if(lstat(root,&st)||!S_ISDIR(st.st_mode))return -1;
 a=pair_file(key);b=pair_file(volume);c=pair_file(legacy);
 if(a<0||b<0||c<0)return -1;
 if(b&&c)return 2;
 return a&&(b||c)?1:0;
}
