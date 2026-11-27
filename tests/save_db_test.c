#include <sqlite3.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
typedef int (*sg_db_cb)(const char *,const char *,const char *,int64_t,void *);
int sg_db_inventory(const char *,sg_db_cb,void *);
static int row(const char *t,const char *d,const char *n,int64_t b,void *ctx){
 assert(!strcmp(t,"CUSA12345")&&!strcmp(d,"slot")&&!strcmp(n,"Test")&&b==32);
 ++*(int *)ctx;return 0;
}
int main(void){
 char p[]="/tmp/sg-db-XXXXXX";int fd=mkstemp(p);assert(fd>=0);close(fd);
 sqlite3 *db=NULL;assert(sqlite3_open(p,&db)==SQLITE_OK);
 assert(sqlite3_exec(db,"CREATE TABLE savedata(title_id,dir_name,main_title,blocks); INSERT INTO savedata VALUES('CUSA12345','slot','Test',32);",NULL,NULL,NULL)==SQLITE_OK);
 sqlite3_close(db);int n=0;assert(sg_db_inventory(p,row,&n)==1&&n==1);
 unlink(p);assert(sg_db_inventory(p,row,&n)==-1);assert(access(p,F_OK)!=0);return 0;
}
