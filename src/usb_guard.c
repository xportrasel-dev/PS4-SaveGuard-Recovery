#include <stdio.h>
#include <sys/stat.h>
/* Conservative mount boundary check; hardware identity is not established. */
int sg_usb_ready(unsigned usb){
 if(usb>7)return -1;
 char p[64];struct stat root,parent,internal;
 snprintf(p,sizeof p,"/mnt/usb%u",usb);
 if(lstat(p,&root)||!S_ISDIR(root.st_mode)||stat("/mnt",&parent)||stat("/user",&internal))return -1;
 if(root.st_dev==parent.st_dev||root.st_dev==internal.st_dev)return -1;
 return 0;
}
