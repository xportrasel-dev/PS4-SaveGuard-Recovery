#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
int sg_verify_file(const char *,const unsigned char [32]);
static int hex(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;return -1;}
static int regular_path(const char *root,const char *rel,char out[8192]){
 if(!*rel||*rel=='/'||strlen(rel)>4095)return -1;
 int n=snprintf(out,8192,"%s/data",root);if(n<0||n>=8192)return -1;
 struct stat st;if(lstat(out,&st)||!S_ISDIR(st.st_mode))return -1;
 while(*rel){
 const char *slash=strchr(rel,'/');size_t len=slash?(size_t)(slash-rel):strlen(rel);
 if(!len||(len==1&&rel[0]=='.')||(len==2&&!memcmp(rel,"..",2))||n+1+len>=8192)return -1;
 out[n++]='/';memcpy(out+n,rel,len);n+=(int)len;out[n]=0;
 if(lstat(out,&st))return -1;
 if(!slash)return S_ISREG(st.st_mode)?0:-1;
 if(!S_ISDIR(st.st_mode))return -1;
 rel=slash+1;if(!*rel)return -1;
 }return -1;
}
int sg_verify_snapshot(const char *root,unsigned *checked){
 char path[8192],line[4200];struct stat st;unsigned count=0;
 if(checked)*checked=0;
 if(!root||strlen(root)>4000||lstat(root,&st)||!S_ISDIR(st.st_mode))return -1;
 snprintf(path,sizeof path,"%s/.complete",root);
 if(lstat(path,&st)||!S_ISREG(st.st_mode))return -1;
 snprintf(path,sizeof path,"%s/checksums.sha256",root);
 if(lstat(path,&st)||!S_ISREG(st.st_mode))return -1;
 FILE *f=fopen(path,"r");if(!f)return -1;
 int rc=0;
 while(fgets(line,sizeof line,f)){
 size_t len=strlen(line);unsigned char digest[32];
 if(len<68||line[len-1]!='\n'||line[64]!=' '||line[65]!=' '){rc=-1;break;}
 line[len-1]=0;
 for(int i=0;i<32;i++){int a=hex(line[2*i]),b=hex(line[2*i+1]);if(a<0||b<0){rc=-1;break;}digest[i]=(unsigned char)(16*a+b);}
 if(rc||regular_path(root,line+66,path)||sg_verify_file(path,digest)){rc=-1;break;}
 count++;
 }
 if(ferror(f)||!count)rc=-1;
 fclose(f);if(checked)*checked=count;return rc;
}
