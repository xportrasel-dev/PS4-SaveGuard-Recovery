#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
int sg_export(const char *,const char *);
int main(void){
 char dir[]="/tmp/sg-test-XXXXXX"; assert(mkdtemp(dir));
 char src[256],dst[256],link[256];
 snprintf(src,sizeof src,"%s/source",dir);
 snprintf(dst,sizeof dst,"%s/dest",dir);
 snprintf(link,sizeof link,"%s/link",dir);
 FILE *f=fopen(src,"wb"); assert(f);
 assert(fwrite("save",1,4,f)==4); assert(!fclose(f));
 assert(sg_export(src,dst)==0);
 assert(sg_export(src,dst)!=0);
 f=fopen(dst,"rb"); assert(f); char b[5]={0};
 assert(fread(b,1,4,f)==4); assert(!strcmp(b,"save")); fclose(f);
 assert(!symlink(src,link)); assert(sg_export(link,dst)!=0);
 unlink(link); unlink(dst); unlink(src); rmdir(dir);
 puts("export tests passed"); return 0;
}
