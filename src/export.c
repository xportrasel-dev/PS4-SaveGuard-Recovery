#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

/* Caller must select an external destination and ensure stable source.
 * No source modification. Exclusive destination creation. */
int sg_export(const char *src, const char *dst) {
    int a=-1,b=-1,v=-1,ok=-1,created=0;
    struct stat st,after;
    unsigned char buf[16384], check[16384];
    ssize_t n;
    a=open(src,O_RDONLY|O_NOFOLLOW);
    if(a<0 || fstat(a,&st) || !S_ISREG(st.st_mode)) goto done;
    b=open(dst,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
    if(b<0) goto done;
    created=1;
    while((n=read(a,buf,sizeof buf))!=0) {
        if(n<0) { if(errno==EINTR) continue; goto done; }
        ssize_t off=0;
        while(off<n) {
            ssize_t w=write(b,buf+off,n-off);
            if(w<0 && errno==EINTR) continue;
            if(w<=0) goto done;
            off+=w;
        }
    }
    if(fsync(b)) goto done;
    if(close(b)) { b=-1; goto done; }
    b=-1;
    v=open(dst,O_RDONLY|O_NOFOLLOW);
    if(v<0 || lseek(a,0,SEEK_SET)<0) goto done;
    while((n=read(a,buf,sizeof buf))!=0) {
        if(n<0) { if(errno==EINTR) continue; goto done; }
        ssize_t off=0;
        while(off<n) {
            ssize_t r=read(v,check+off,n-off);
            if(r<0 && errno==EINTR) continue;
            if(r<=0) goto done;
            off+=r;
        }
        if(memcmp(buf,check,n)) goto done;
    }
    if(read(v,check,1)!=0 || fstat(a,&after) ||
       st.st_size!=after.st_size || st.st_mtime!=after.st_mtime) goto done;
    ok=0;
done:
    if(a>=0) close(a);
    if(b>=0) close(b);
    if(v>=0) close(v);
    if(ok && created) unlink(dst);
    return ok;
}
