#include <string.h>
#include <stdint.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#define SG_PIN_ROUNDS 200000
struct sg_pin_record {unsigned char salt[16],hash[32];};
static int derive(const char *pin,const unsigned char salt[16],unsigned char out[32]){
 if(!pin||!salt||!out)return -1;
 size_t n=strlen(pin);if(n<6||n>12)return -1;
 for(size_t i=0;i<n;i++)if(pin[i]<'0'||pin[i]>'9')return -1;
 mbedtls_md_context_t ctx;mbedtls_md_init(&ctx);
 const mbedtls_md_info_t *md=mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
 int rc=md?mbedtls_md_setup(&ctx,md,1):-1;
 if(!rc)rc=mbedtls_pkcs5_pbkdf2_hmac(&ctx,(const unsigned char *)pin,n,salt,16,SG_PIN_ROUNDS,32,out);
 mbedtls_md_free(&ctx);return rc?-1:0;
}
/* Caller must supply a fresh cryptographically random salt. */
int sg_pin_create(struct sg_pin_record *r,const char *pin,const unsigned char salt[16]){
 if(!r||!salt)return -1;
 struct sg_pin_record tmp;
 memcpy(tmp.salt,salt,16);
 if(derive(pin,tmp.salt,tmp.hash))return -1;
 *r=tmp;return 0;
}
int sg_pin_verify(const struct sg_pin_record *r,const char *pin){
 unsigned char h[32];unsigned diff=0;
 if(!r||derive(pin,r->salt,h))return -1;
 for(unsigned i=0;i<32;i++)diff|=h[i]^r->hash[i];
 volatile unsigned char *p=h;for(unsigned i=0;i<32;i++)p[i]=0;
 return diff?-1:0;
}

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
int sg_pin_create_random(struct sg_pin_record *r,const char *pin){
 unsigned char salt[16];size_t off=0;
 int fd=open("/dev/urandom",O_RDONLY);
 if(fd<0)return -1;
 while(off<sizeof salt){
 ssize_t n=read(fd,salt+off,sizeof salt-off);
 if(n<0&&errno==EINTR)continue;
 if(n<=0)break;
 off+=(size_t)n;
 }
 close(fd);
 if(off!=sizeof salt)return -1;
 return sg_pin_create(r,pin,salt);
}
