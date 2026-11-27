#include <SDL2/SDL.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
int sg_snapshot(const char *,unsigned);
static void rect(SDL_Renderer *r,int x,int y,int w,int h,int a,int b,int c){
 SDL_Rect q={x,y,w,h};SDL_SetRenderDrawColor(r,a,b,c,255);SDL_RenderFillRect(r,&q);
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
 int running=1,selected=0;
 while(running){
 SDL_Event e;
 while(SDL_PollEvent(&e)){
 if(e.type==SDL_QUIT)running=0;
 if(e.type==SDL_CONTROLLERBUTTONDOWN){
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_DOWN)selected=(selected+1)%4;
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_UP)selected=(selected+3)%4;
 if(e.cbutton.button==SDL_CONTROLLER_BUTTON_B)running=0;
 }
 }
 SDL_SetRenderDrawColor(r,12,20,36,255);SDL_RenderClear(r);
 rect(r,40,40,1200,85,26,47,75);
 for(int i=0;i<4;i++)rect(r,40,150+i*125,350,105,i==selected?40:25,i==selected?110:40,i==selected?185:65);
 rect(r,420,150,820,480,20,34,54);
 SDL_RenderPresent(r);SDL_Delay(16);
 }
 if(pad)SDL_GameControllerClose(pad);
 SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();return 0;
}
