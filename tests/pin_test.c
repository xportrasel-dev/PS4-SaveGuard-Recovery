#include <assert.h>
#include <stdint.h>
struct sg_pin_record {unsigned char salt[16],hash[32];};
struct sg_pin_guard {unsigned failures,authorized;uint64_t blocked_until,expires;};
int sg_pin_create_random(struct sg_pin_record *,const char *);
int sg_pin_verify(const struct sg_pin_record *,const char *);
int sg_guard_attempt(struct sg_pin_guard *,const struct sg_pin_record *,const char *,uint64_t);
int sg_guard_consume(struct sg_pin_guard *,uint64_t);
int main(void){
 struct sg_pin_record r;struct sg_pin_guard g={0};
 assert(sg_pin_create_random(&r,"123")!=0);
 assert(sg_pin_create_random(&r,"123456")==0);
 assert(sg_pin_verify(&r,"123456")==0);
 for(unsigned i=0;i<5;i++)assert(sg_guard_attempt(&g,&r,"654321",100+i)!=0);
 assert(sg_guard_attempt(&g,&r,"123456",105)!=0);
 assert(sg_guard_attempt(&g,&r,"123456",165)==0);
 assert(sg_guard_consume(&g,166)==0);
 assert(sg_guard_consume(&g,167)!=0);
 assert(sg_guard_attempt(&g,&r,"123456",200)==0);
 assert(sg_guard_consume(&g,230)!=0);
 return 0;
}
