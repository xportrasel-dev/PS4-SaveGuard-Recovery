#include <stdint.h>
struct sg_pin_record {unsigned char salt[16],hash[32];};
int sg_pin_verify(const struct sg_pin_record *,const char *);
struct sg_pin_guard {unsigned failures,authorized;uint64_t blocked_until,expires;};
int sg_guard_attempt(struct sg_pin_guard *g,const struct sg_pin_record *r,const char *pin,uint64_t now){
 if(!g||!r)return -1;
 g->authorized=0;
 if(now<g->blocked_until)return -2;
 if(sg_pin_verify(r,pin)){
 if(g->failures<5)g->failures++;
 if(g->failures>=5)g->blocked_until=now>UINT64_MAX-60?UINT64_MAX:now+60;
 return -1;
 }
 g->failures=0;g->blocked_until=0;
 g->expires=now>UINT64_MAX-30?UINT64_MAX:now+30;
 g->authorized=1;return 0;
}
int sg_guard_consume(struct sg_pin_guard *g,uint64_t now){
 if(!g)return -1;
 int ok=g->authorized&&now<g->expires;
 g->authorized=0;return ok?0:-1;
}
void sg_guard_cancel(struct sg_pin_guard *g){if(g)g->authorized=0;}
