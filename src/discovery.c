#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

/* Read-only discovery; not deleted-sector recovery. */
static unsigned count;
static void scan(const char *path, unsigned depth) {
    if (!depth) return;
    DIR *d = opendir(path);
    if (!d) { printf("Unavailable: %s\n", path); return; }
    struct dirent *e;
    while ((e = readdir(d))) {
        if (!strcmp(e->d_name,".") || !strcmp(e->d_name,"..")) continue;
        char p[4096];
        int n = snprintf(p,sizeof p,"%s/%s",path,e->d_name);
        if (n < 0 || (size_t)n >= sizeof p) continue;
        struct stat st;
        if (lstat(p,&st)) continue;
        if (S_ISDIR(st.st_mode)) scan(p,depth-1);
        else if (S_ISREG(st.st_mode)) { printf("%s (%lld bytes)\n",p,(long long)st.st_size); ++count; }
    }
    closedir(d);
}
int main(void) {
    scan("/user/home",6);
    scan("/data/apollo",5);
    scan("/data/fakeusb/PS4",5);
    printf("Existing files discovered: %u\n",count);
    return 0;
}
