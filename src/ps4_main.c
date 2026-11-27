#include <SDL2/SDL.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
int sg_snapshot(const char *,unsigned);
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
 while(running){
 SDL_Event e;
 while(SDL_PollEvent(&e)){
 if(e.type==SDL_QUIT)running=0;
 if(e.type==SDL_CONTROLLERBUTTONDOWN){
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_DOWN)selected=(selected+1)%7;
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_UP)selected=(selected+6)%7;
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
 else running=0;
 }
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_UP||e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_DOWN)confirm=0;
 }
 }
 SDL_SetRenderDrawColor(r,12,20,36,255);SDL_RenderClear(r);
 rect(r,40,40,1200,85,26,47,75);
 for(int i=0;i<7;i++)rect(r,40,150+i*68,350,60,i==selected?40:25,i==selected?110:40,i==selected?185:65);
 rect(r,420,150,820,480,20,34,54);
 rect(r,450,180,760,24,status==2?190:40,status==1?170:65,80);
 if(confirm)rect(r,450,230,760,45,190,120,30);
 for(unsigned i=0;i<8;i++)rect(r,450+i*90,300,70,35,i==usb?40:30,i==usb?150:60,100);
 label(r,65,65,5,"PS4 SAVEGUARD");
 const char *names[]={"SCAN FILES","USB BACKUP","NEXT GAME","HISTORY","VERIFY BACKUP","STAGE RECOVERY","EXIT"};
 for(int i=0;i<7;i++)label(r,65,168+i*68,3,names[i]);
 char info[100];snprintf(info,sizeof info,"FILES %u   ERRORS %u",files,errors);
 label(r,450,345,3,info);
 label(r,450,390,3,status==2?"ERROR OR PARTIAL":status==1?"OPERATION COMPLETE":"READY");
 label(r,450,440,3,confirm?"PRESS X AGAIN TO COPY":"X SELECT   O BACK");
 label(r,450,485,2,"LEFT RIGHT SELECT USB");
 label(r,450,530,2,"EXISTING FILES ONLY");
 if(game_count){snprintf(info,sizeof info,"GAME %u OF %u  %s",game_index+1,game_count,games[game_index].title);label(r,450,570,3,info);}else label(r,450,570,3,"NO GAMES SCANNED");
 for(unsigned i=0;i<8;i++){char num[2]={(char)('0'+i),0};label(r,475+i*90,308,2,num);}
 if(selected==3||selected==4||selected==5){
 if(backup_count){snprintf(info,sizeof info,"BACKUP %u OF %u %s",backup_index+1,backup_count,backups[backup_index].title);label(r,450,610,2,info);}
 else label(r,450,610,2,"NO BACKUPS LOADED");
 }
 if(selected==4){snprintf(info,sizeof info,"VERIFIED FILES %u",verified_files);label(r,450,655,2,info);}
 SDL_RenderPresent(r);SDL_Delay(16);
 }
 if(pad)SDL_GameControllerClose(pad);
 SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();return 0;
}
