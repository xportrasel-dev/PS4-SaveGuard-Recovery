#include <stddef.h>
typedef void (*sg_catalog_cb)(const char *,const char *,const char *,void *);
int sg_catalog(sg_catalog_cb,void *);
int sg_snapshot(const char *,unsigned);
struct sg_batch_result {unsigned usb,attempted,succeeded,failed;};
static void backup_game(const char *user,const char *title,const char *path,void *opaque){
 (void)user;(void)title;
 struct sg_batch_result *r=opaque;
 r->attempted++;
 if(sg_snapshot(path,r->usb))r->failed++;else r->succeeded++;
}
/* Explicit invocation only; never run at startup or in the background. */
int sg_batch_backup(unsigned usb,struct sg_batch_result *r){
 if(!r||usb>7)return -1;
 *r=(struct sg_batch_result){.usb=usb};
 int n=sg_catalog(backup_game,r);
 return n<0||!r->attempted||r->failed?-1:0;
}
