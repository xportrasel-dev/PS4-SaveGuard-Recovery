#include <SDL2/SDL.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
int sg_snapshot(const char *,unsigned);
int sg_usb_ready(unsigned);
static void rect(SDL_Renderer *r,int x,int y,int w,int h,int a,int b,int c){
 SDL_Rect q={x,y,w,h};SDL_SetRenderDrawColor(r,a,b,c,255);SDL_RenderFillRect(r,&q);
}
static unsigned files,errors;
typedef void (*sg_catalog_cb)(const char *,const char *,const char *,void *);
int sg_catalog(sg_catalog_cb,void *);
static struct {char user[9],title[10],path[256];} games[512];
static unsigned game_count,game_index;
typedef void (*sg_history_cb)(const char *,const char *,const char *,long long,void *);
int sg_history(unsigned,sg_history_cb,void *);
static struct {char title[10],path[512];long long stamp;} backups[512];
static unsigned backup_count,backup_index,verified_files;
int sg_verify_snapshot(const char *,unsigned *);
int sg_stage_restore(const char *);
struct sg_batch_result {unsigned usb,attempted,succeeded,failed;};
int sg_batch_backup(unsigned,struct sg_batch_result *);
static struct sg_batch_result batch;
struct sg_schedule {uint64_t interval_seconds,next_due;unsigned enabled,usb,running;};
int sg_schedule_configure(struct sg_schedule *,unsigned,unsigned,uint64_t,uint64_t);
int sg_schedule_due(struct sg_schedule *,uint64_t);
void sg_schedule_finish(struct sg_schedule *,uint64_t,int);
static struct sg_schedule schedule;
struct sg_pin_record {unsigned char salt[16],hash[32];};
struct sg_pin_guard {unsigned failures,authorized;uint64_t blocked_until,expires;};
int sg_pin_create_random(struct sg_pin_record *,const char *);
int sg_pin_store_new(unsigned,const struct sg_pin_record *);
int sg_pin_load(unsigned,struct sg_pin_record *);
int sg_guard_attempt(struct sg_pin_guard *,const struct sg_pin_record *,const char *,uint64_t);
static struct sg_pin_guard guard;
static struct sg_pin_record pin_record;
static char pin_input[13];
static unsigned pin_len,pin_digit;
static int pin_mode;
int sg_trash_move(unsigned,const char *,int);
int sg_guard_consume(struct sg_pin_guard *,uint64_t);
static int pending_trash;
static void clear_pin(void){volatile char *p=pin_input;for(unsigned i=0;i<13;i++)p[i]=0;pin_len=pin_digit=0;}
int sg_settings_save(const struct sg_schedule *);
int sg_settings_load(struct sg_schedule *,unsigned,uint64_t);
static uint64_t now_seconds(void){time_t t=time(NULL);return t<0?0:(uint64_t)t;}
static void add_backup(const char *p,const char *u,const char *t,long long stamp,void *ctx){
 (void)u;(void)ctx;
 if(backup_count>=512)return;
 snprintf(backups[backup_count].title,10,"%s",t);
 snprintf(backups[backup_count].path,512,"%s",p);
 backups[backup_count++].stamp=stamp;
}
static void add_game(const char *u,const char *t,const char *p,void *ctx){
 (void)ctx;
 if(game_count>=512){errors++;return;}
 snprintf(games[game_count].user,9,"%s",u);
 snprintf(games[game_count].title,10,"%s",t);
 snprintf(games[game_count].path,256,"%s",p);
 game_count++;
}
static void inventory(const char *p,unsigned depth){
 if(!depth){errors++;return;}
 DIR *d=opendir(p);if(!d){errors++;return;}
 struct dirent *e;
 while((e=readdir(d))){
 if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
 char q[4096];struct stat st;
 int n=snprintf(q,sizeof q,"%s/%s",p,e->d_name);
 if(n<0||(size_t)n>=sizeof q||lstat(q,&st)){errors++;continue;}
 if(S_ISDIR(st.st_mode))inventory(q,depth-1);
 else if(S_ISREG(st.st_mode))files++;
 }
 closedir(d);
}
/* Five-column bitmap font, uppercase ASCII and digits. */
static const unsigned char glyph[36][5]={
{126,9,9,9,126},{127,73,73,73,54},{62,65,65,65,34},{127,65,65,34,28},{127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},{127,8,8,8,127},{65,65,127,65,65},{32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},{127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},{127,9,9,9,6},{62,65,81,33,94},{127,9,25,41,70},{38,73,73,73,50},{1,1,127,1,1},{63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},{7,8,112,8,7},{97,81,73,69,67},
{62,81,73,69,62},{0,66,127,64,0},{98,81,73,73,70},{34,65,73,73,54},{24,20,18,127,16},{39,69,69,69,57},{62,73,73,73,50},{1,113,9,5,3},{54,73,73,73,54},{38,73,73,73,62}};
static void label(SDL_Renderer *r,int x,int y,int scale,const char *s){
 for(;*s;s++,x+=6*scale){
 int k=*s>='A'&&*s<='Z'?*s-'A':*s>='0'&&*s<='9'?26+*s-'0':-1;
 if(k<0)continue;
 for(int i=0;i<5;i++)for(int j=0;j<7;j++)if(glyph[k][i]&(1<<j))rect(r,x+i*scale,y+j*scale,scale,scale,230,240,250);
 }
}
/* Prototype visual dashboard. No internal disk writes or automatic backup. */
int main(void){
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER))return 1;
 SDL_Window *w=SDL_CreateWindow("SaveGuard",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,1280,720,0);
 if(!w){SDL_Quit();return 1;}
 SDL_Renderer *r=SDL_CreateRenderer(w,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
 if(!r){SDL_DestroyWindow(w);SDL_Quit();return 1;}
 SDL_GameController *pad=NULL;
 for(int i=0;i<SDL_NumJoysticks();i++)if(SDL_IsGameController(i)){pad=SDL_GameControllerOpen(i);if(pad)break;}
 int running=1,selected=0,status=0,confirm=0; unsigned usb=0;
 sg_settings_load(&schedule,usb,now_seconds());
 while(running){
 SDL_Event e;
 while(SDL_PollEvent(&e)){
 if(e.type==SDL_QUIT)running=0;
 if(e.type==SDL_CONTROLLERBUTTONDOWN){
 if(pin_mode){
 unsigned b=e.cbutton.button;
 if(b==SDL_CONTROLLER_BUTTON_DPAD_UP)pin_digit=(pin_digit+1)%10;
 if(b==SDL_CONTROLLER_BUTTON_DPAD_DOWN)pin_digit=(pin_digit+9)%10;
 if(b==SDL_CONTROLLER_BUTTON_A&&pin_len<12){pin_input[pin_len++]=(char)('0'+pin_digit);pin_input[pin_len]=0;}
 if(b==SDL_CONTROLLER_BUTTON_DPAD_LEFT&&pin_len)pin_input[--pin_len]=0;
 if(b==SDL_CONTROLLER_BUTTON_B){clear_pin();pin_mode=0;pending_trash=0;}
 if(b==SDL_CONTROLLER_BUTTON_Y&&pin_len>=6){
 int rc=pin_mode==1?(sg_pin_create_random(&pin_record,pin_input)||sg_pin_store_new(usb,&pin_record)):sg_guard_attempt(&guard,&pin_record,pin_input,now_seconds());
 if(!rc&&pending_trash){
 const char *name=backup_count?strrchr(backups[backup_index].path,'/'):NULL;
 rc=sg_guard_consume(&guard,now_seconds())||!name||sg_trash_move(usb,name+1,0);
 if(!rc)backup_count=backup_index=0;
 }
 pending_trash=0;status=rc?2:1;clear_pin();pin_mode=0;
 }
 continue;
 }
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_DOWN)selected=(selected+1)%11;
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_UP)selected=(selected+10)%11;
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_B){if(confirm)confirm=0;else running=0;}
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_LEFT){usb=(usb+7)%8;confirm=0;backup_count=backup_index=0;}
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_RIGHT){usb=(usb+1)%8;confirm=0;backup_count=backup_index=0;}
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_A){
 if(selected==0){files=errors=0;game_count=game_index=0;if(sg_catalog(add_game,NULL)<0)errors++;for(unsigned i=0;i<game_count;i++)inventory(games[i].path,12);status=errors?2:1;confirm=0;}
 else if(selected==1){if(!game_count){status=2;confirm=0;}else if(confirm){status=sg_snapshot(games[game_index].path,usb)?2:1;confirm=0;}else confirm=1;}
 else if(selected==2){if(game_count)game_index=(game_index+1)%game_count;status=0;confirm=0;}
 else if(selected==3){if(backup_count)backup_index=(backup_index+1)%backup_count;else {backup_index=0;status=sg_history(usb,add_backup,NULL)<0?2:1;}}
 else if(selected==4){verified_files=0;status=backup_count&&!sg_verify_snapshot(backups[backup_index].path,&verified_files)?1:2;}
 else if(selected==5){if(!backup_count){status=2;confirm=0;}else if(confirm){status=sg_stage_restore(backups[backup_index].path)?2:1;confirm=0;}else confirm=1;}
 else if(selected==6){if(confirm){status=sg_batch_backup(usb,&batch)?2:1;confirm=0;backup_count=backup_index=0;}else confirm=1;}
 else if(selected==7){if(schedule.enabled){schedule.enabled=0;confirm=0;status=sg_settings_save(&schedule)?2:1;}else if(confirm){status=sg_schedule_configure(&schedule,1,usb,86400,now_seconds())?2:1;if(status==1&&sg_settings_save(&schedule)){schedule.enabled=0;status=2;}confirm=0;}else confirm=1;}
 else if(selected==8){clear_pin();pin_mode=sg_pin_load(usb,&pin_record)==0?2:1;}
 else if(selected==9){
 if(!backup_count||sg_pin_load(usb,&pin_record)){status=2;confirm=0;}
 else if(confirm){clear_pin();pending_trash=1;pin_mode=2;confirm=0;}
 else confirm=1;
 }
 else running=0;
 }
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_UP||e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_DOWN)confirm=0;
 }
 }
 if(running&&!confirm&&!pin_mode&&sg_schedule_due(&schedule,now_seconds())){
 int rc=sg_batch_backup(schedule.usb,&batch);
 sg_schedule_finish(&schedule,now_seconds(),rc==0);
 status=rc?2:1;backup_count=backup_index=0;
 }
 SDL_SetRenderDrawColor(r,12,20,36,255);SDL_RenderClear(r);
 rect(r,40,40,1200,85,26,47,75);
 for(int i=0;i<11;i++)rect(r,40,150+i*47,350,42,i==selected?40:25,i==selected?110:40,i==selected?185:65);
 rect(r,420,150,820,480,20,34,54);
 rect(r,450,180,760,24,status==2?190:40,status==1?170:65,80);
 if(confirm)rect(r,450,230,760,45,190,120,30);
 for(unsigned i=0;i<8;i++)rect(r,450+i*90,300,70,35,i==usb?40:30,i==usb?150:60,100);
 label(r,65,65,5,"PS4 SAVEGUARD");
 const char *names[]={"SCAN FILES","USB BACKUP","NEXT GAME","HISTORY","VERIFY BACKUP","STAGE RECOVERY","BACKUP ALL","AUTO BACKUP","PIN SECURITY","TRASH BACKUP","EXIT"};
 for(int i=0;i<11;i++)label(r,65,158+i*47,3,names[i]);
 char info[100];snprintf(info,sizeof info,"FILES %u   ERRORS %u",files,errors);
 label(r,450,345,3,info);
 label(r,450,390,3,status==2?"ERROR OR PARTIAL":status==1?"OPERATION COMPLETE":"READY");
 label(r,450,440,3,confirm?(selected==7?"PRESS X TO ENABLE AUTO":selected==9?"PRESS X TO MOVE TO TRASH":"PRESS X AGAIN TO COPY"):"X SELECT   O BACK");
 label(r,450,485,2,"LEFT RIGHT SELECT USB");
 label(r,450,510,2,sg_usb_ready(usb)==0?"USB MOUNT DETECTED":"USB NOT READY");
 label(r,450,540,2,"EXISTING FILES ONLY");
 if(game_count){snprintf(info,sizeof info,"GAME %u OF %u  %s",game_index+1,game_count,games[game_index].title);label(r,450,570,3,info);}else label(r,450,570,3,"NO GAMES SCANNED");
 for(unsigned i=0;i<8;i++){char num[2]={(char)('0'+i),0};label(r,475+i*90,308,2,num);}
 if(selected==3||selected==4||selected==5||selected==9){
 if(backup_count){snprintf(info,sizeof info,"BACKUP %u OF %u %s",backup_index+1,backup_count,backups[backup_index].title);label(r,450,610,2,info);}
 else label(r,450,610,2,"NO BACKUPS LOADED");
 }
 if(selected==4){snprintf(info,sizeof info,"VERIFIED FILES %u",verified_files);label(r,450,655,2,info);}
 if(selected==6){snprintf(info,sizeof info,"TOTAL %u OK %u FAILED %u",batch.attempted,batch.succeeded,batch.failed);label(r,450,610,2,info);}
 if(selected==7){label(r,450,610,2,schedule.enabled?"AUTO BACKUP ON":"AUTO BACKUP OFF");snprintf(info,sizeof info,"INTERVAL 24 HOURS USB %u",schedule.enabled?schedule.usb:usb);label(r,450,650,2,info);}
 if(pin_mode){
 rect(r,420,150,820,520,20,34,54);
 label(r,450,190,3,pin_mode==1?"CREATE PIN":"VERIFY PIN");
 snprintf(info,sizeof info,"DIGIT %u",pin_digit);label(r,450,250,4,info);
 for(unsigned i=0;i<pin_len;i++)rect(r,450+i*32,330,20,20,230,240,250);
 label(r,450,390,2,"UP DOWN CHANGE DIGIT");
 label(r,450,430,2,"X ADD   LEFT REMOVE");
 label(r,450,470,2,"TRIANGLE SUBMIT   O CANCEL");
 label(r,450,510,2,"6 TO 12 DIGITS");
 }
 SDL_RenderPresent(r);SDL_Delay(16);
 }
 if(pad)SDL_GameControllerClose(pad);
 SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();return 0;
}
