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
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_DOWN)selected=(selected+1)%4;
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_UP)selected=(selected+3)%4;
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_B){if(confirm)confirm=0;else running=0;}
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_LEFT){usb=(usb+7)%8;confirm=0;}
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_RIGHT){usb=(usb+1)%8;confirm=0;}
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_A){
 if(selected==0){files=errors=0;inventory("/user/home",12);status=errors?2:1;confirm=0;}
 else if(selected==1){if(confirm){status=sg_snapshot("/user/home",usb)?2:1;confirm=0;}else confirm=1;}
 else if(selected==2){status=0;confirm=0;}
 else running=0;
 }
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_UP||e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_DOWN)confirm=0;
 }
 }
 SDL_SetRenderDrawColor(r,12,20,36,255);SDL_RenderClear(r);
 rect(r,40,40,1200,85,26,47,75);
 for(int i=0;i<4;i++)rect(r,40,150+i*125,350,105,i==selected?40:25,i==selected?110:40,i==selected?185:65);
 rect(r,420,150,820,480,20,34,54);
 rect(r,450,180,760,24,status==2?190:40,status==1?170:65,80);
 if(confirm)rect(r,450,230,760,45,190,120,30);
 for(unsigned i=0;i<8;i++)rect(r,450+i*90,300,70,35,i==usb?40:30,i==usb?150:60,100);
 SDL_RenderPresent(r);SDL_Delay(16);
 }
 if(pad)SDL_GameControllerClose(pad);
 SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();return 0;
}
