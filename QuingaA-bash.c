#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

void imprimir(const char *s){
    int t=0; while(s[t]) t++; write(1,s,t);
}
void imprimirln(const char *s){
    imprimir(s); write(1,"\n",1);
}
int normalize(char *in, char *out, int osz){
    int i=0,j=0;
    while(in[i] && j+2 < osz){
        if(in[i]=='>' && in[i+1]=='>'){
            if(j>0 && out[j-1]!=' ') out[j++]=' ';
            out[j++]='>'; out[j++]='>';
            if(in[i+2] && in[i+2]!=' ') out[j++]=' ';
            i+=2;
        } else if(in[i]=='<' || in[i]=='>'){
            if(j>0 && out[j-1]!=' ') out[j++]=' ';
            out[j++]=in[i++];
            if(in[i] && in[i]!=' ') out[j++]=' ';
        } else {
            out[j++]=in[i++];
        }
    }
    out[j]=0;
    return 0;
}
int tokenizar(char *c, char *t[], int max){
    int i=0,n=0;
    while(c[i]){
        while(c[i]==' '||c[i]=='\t') i++;
        if(!c[i]) break;
        if(n>=max-1) break;
        t[n++]=&c[i];
        while(c[i] && c[i]!=' ' && c[i]!='\t') i++;
        if(c[i]){ c[i]=0; i++; }
    }
    t[n]=0;
    return n;
}
int es_background(char *t[], int nt){
    if(nt==0) return 0;
    char *x=t[nt-1];
    return (x[0]=='&' && x[1]==0);
}
int construir_path(char *t[], char p[], int psz){
    char *dirs[]={"/bin/","/usr/bin/"};
    for(int d=0; d<2; d++){
        int a=0,b=0;
        while(dirs[d][a]){ if(b+1<psz) p[b++]=dirs[d][a++]; else return -1; }
        a=0;
        while(t[0][a]){ if(b+1<psz) p[b++]=t[0][a++]; else return -1; }
        p[b]=0;
        if(access(p, X_OK)==0) return 0;
    }
    return -1;
}
int manejar_redirecciones(char *t[], int *nt){
    for(int i=0;i<*nt;i++){
        if(t[i][0]=='<' && t[i][1]==0){
            if(i+1>=*nt) return -1;
            int fd=open(t[i+1],O_RDONLY);
            if(fd<0) return -1;
            dup2(fd,0); close(fd);
            for(int j=i;j+2<=*nt;j++) t[j]=t[j+2];
            *nt -= 2; i--;
        } else if(t[i][0]=='>' && t[i][1]==0){
            if(i+1>=*nt) return -1;
            int fd=open(t[i+1], O_WRONLY|O_CREAT|O_TRUNC, 0644);
            if(fd<0) return -1;
            dup2(fd,1); close(fd);
            for(int j=i;j+2<=*nt;j++) t[j]=t[j+2];
            *nt -= 2; i--;
        } else if(t[i][0]=='>' && t[i][1]=='>' && t[i][2]==0){
            if(i+1>=*nt) return -1;
            int fd=open(t[i+1], O_WRONLY|O_CREAT|O_APPEND, 0644);
            if(fd<0) return -1;
            dup2(fd,1); close(fd);
            for(int j=i;j+2<=*nt;j++) t[j]=t[j+2];
            *nt -= 2; i--;
        }
    }
    t[*nt]=0;
    return 0;
}
int builtin(char *t[], int nt){
    if(nt==0) return 0;
    if(t[0][0]=='c'&&t[0][1]=='d'&&t[0][2]==0){
        if(nt<2){ chdir("/"); return 1; }
        chdir(t[1]);
        return 1;
    }
    if(t[0][0]=='p'&&t[0][1]=='w'&&t[0][2]=='d'&&t[0][3]==0){
        char buf[512]; int n = readlink("/proc/self/cwd", buf, 511);
        if(n>0){ buf[n]=0; imprimirln(buf); }
        return 1;
    }
    if(t[0][0]=='m'&&t[0][1]=='k'&&t[0][2]=='d'&&t[0][3]=='i'&&t[0][4]=='r'&&t[0][5]==0){
        if(nt<2) return 1;
        mkdir(t[1], 0755);
        return 1;
    }
    if(t[0][0]=='r'&&t[0][1]=='m'&&t[0][2]==0){
        if(nt<2) return 1;
        if(unlink(t[1])!=0) rmdir(t[1]);
        return 1;
    }
    if(t[0][0]=='m'&&t[0][1]=='v'&&t[0][2]==0){
        if(nt<3) return 1;
        rename(t[1], t[2]);
        return 1;
    }
    if(t[0][0]=='c'&&t[0][1]=='p'&&t[0][2]==0){
        if(nt<3) return 1;
        int in=open(t[1], O_RDONLY);
        if(in<0) return 1;
        int out=open(t[2], O_WRONLY|O_CREAT|O_TRUNC, 0644);
        if(out<0){ close(in); return 1; }
        char buf[4096]; int r;
        while((r=read(in,buf,4096))>0) write(out,buf,r);
        close(in); close(out);
        return 1;
    }
    if(t[0][0]=='c'&&t[0][1]=='a'&&t[0][2]=='t'&&t[0][3]==0){
        if(nt<2) return 1;
        int f=open(t[1], O_RDONLY);
        if(f<0) return 1;
        char buf[4096]; int r;
        while((r=read(f,buf,4096))>0) write(1,buf,r);
        close(f);
        return 1;
    }
    return 0;
}
int main(){
    char in[512], norm[1024], path[512];
    char *t[64];
    while(1){
        imprimir("Terminal> ");
        int n = read(0,in,511);
        if(n<=0) break;
        in[n]=0;
        if(in[n-1]=='\n') in[n-1]=0;
        normalize(in,norm,sizeof(norm));
        int nt = tokenizar(norm,t,64);
        if(nt==0) continue;
        if(t[0][0]=='e'&&t[0][1]=='x'&&t[0][2]=='i'&&t[0][3]=='t'&&t[0][4]==0) break;
        if(builtin(t,nt)) continue;
        int bg = es_background(t,nt);
        if(bg){ t[nt-1]=0; nt--; }
        pid_t pid=fork();
        if(pid==0){
            manejar_redirecciones(t,&nt);
            if(t[0][0]=='/'||t[0][0]=='.') execve(t[0],t,0);
            if(construir_path(t,path,sizeof(path))==0) execve(path,t,0);
            imprimirln("error");
            _exit(1);
        }
        if(!bg) wait(NULL);
    }
    return 0;
}
