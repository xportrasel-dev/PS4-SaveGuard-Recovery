#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <mbedtls/sha256.h>
#include <mbedtls/version.h>
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
#define SG_SHA256_STARTS mbedtls_sha256_starts
#define SG_SHA256_UPDATE mbedtls_sha256_update
#define SG_SHA256_FINISH mbedtls_sha256_finish
#else
#define SG_SHA256_STARTS mbedtls_sha256_starts_ret
#define SG_SHA256_UPDATE mbedtls_sha256_update_ret
#define SG_SHA256_FINISH mbedtls_sha256_finish_ret
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
/* Hash regular files only. Expected digest must come from a trusted manifest. */
int sg_hash_file(const char *path,unsigned char digest[32]){
 int fd=open(path,O_RDONLY|O_NOFOLLOW);if(fd<0)return -1;
 struct stat before,after;
 if(fstat(fd,&before)||!S_ISREG(before.st_mode)){close(fd);return -1;}
 mbedtls_sha256_context ctx;mbedtls_sha256_init(&ctx);
 int rc=SG_SHA256_STARTS(&ctx,0);
 unsigned char buf[16384];ssize_t n;
 while(!rc){
 n=read(fd,buf,sizeof buf);
 if(n<0&&errno==EINTR)continue;
 if(n<0){rc=-1;break;}
 if(!n)break;
 rc=SG_SHA256_UPDATE(&ctx,buf,(size_t)n);
 }
 if(!rc)rc=SG_SHA256_FINISH(&ctx,digest);
 if(fstat(fd,&after)||before.st_size!=after.st_size||before.st_mtime!=after.st_mtime)rc=-1;
 mbedtls_sha256_free(&ctx);close(fd);return rc? -1:0;
}
int sg_verify_file(const char *path,const unsigned char expected[32]){
 unsigned char actual[32];
 if(!expected||sg_hash_file(path,actual))return -1;
 return memcmp(actual,expected,32)?-1:0;
}
