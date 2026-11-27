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
