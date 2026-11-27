#include <stdint.h>
#include <stddef.h>
struct sg_schedule {
 uint64_t interval_seconds,next_due;
 unsigned enabled,usb,running;
};
int sg_schedule_configure(struct sg_schedule *s,unsigned enabled,unsigned usb,uint64_t interval,uint64_t now){
 if(!s||usb>7||interval<300||interval>604800||now>UINT64_MAX-interval||s->running)return -1;
 *s=(struct sg_schedule){interval,now+interval,!!enabled,usb,0};
 return 0;
}
int sg_schedule_due(struct sg_schedule *s,uint64_t now){
 if(!s||!s->enabled||s->running||now<s->next_due)return 0;
 s->running=1;return 1;
}
void sg_schedule_finish(struct sg_schedule *s,uint64_t now,int success){
 if(!s||!s->running)return;
 uint64_t delay=success?s->interval_seconds:300;
 s->running=0;
 if(now>UINT64_MAX-delay){s->enabled=0;return;}
 s->next_due=now+delay;
}
