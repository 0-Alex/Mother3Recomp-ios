// Reference probe: run ROM in libmgba, dump frames + memory snapshots.
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/core/log.h>
#include <mgba/gba/interface.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void nolog(struct mLogger* l, int c, enum mLogLevel lv, const char* f, va_list a){(void)l;(void)c;(void)lv;(void)f;(void)a;}
static struct mLogger L = { .log = nolog };
// input script: lines "frame keysmask"
int main(int argc,char**argv){
  if(argc<4){fprintf(stderr,"probe rom frames outdir [inputs] [shotevery]\n");return 1;}
  mLogSetDefaultLogger(&L);
  struct mCore* c = mCoreFind(argv[1]); if(!c) return 2;
  c->init(c); mCoreInitConfig(c,NULL);
  unsigned w,h; c->desiredVideoDimensions(c,&w,&h);
  uint32_t* fb = malloc(w*h*4); c->setVideoBuffer(c,(void*)fb,w);
  if(!mCoreLoadFile(c,argv[1])) return 3;
  c->reset(c);
  int frames=atoi(argv[2]); const char*out=argv[3];
  int nin=0; int inf[4096], ink[4096];
  if(argc>4 && strcmp(argv[4],"-")){FILE*f=fopen(argv[4],"r"); while(nin<4096&&fscanf(f,"%d %i",&inf[nin],&ink[nin])==2) nin++; fclose(f);}
  int every = argc>5?atoi(argv[5]):60;
  int ii=0; int keys=0; char p[512];
  for(int fr=0;fr<frames;fr++){
    while(ii<nin && inf[ii]<=fr){keys=ink[ii];ii++;}
    c->setKeys(c,keys);
    c->runFrame(c);
    if(every>0 && (fr+1)%every==0){
      snprintf(p,sizeof p,"%s/f%06d.rgba",out,fr+1); FILE*f=fopen(p,"wb"); fwrite(fb,4,w*h,f); fclose(f);
      snprintf(p,sizeof p,"%s/iw%06d.bin",out,fr+1); f=fopen(p,"wb");
      for(uint32_t a=0x03000000;a<0x03008000;a++){uint8_t b=c->rawRead8(c,a,-1);fputc(b,f);} fclose(f);
      snprintf(p,sizeof p,"%s/ew%06d.bin",out,fr+1); f=fopen(p,"wb");
      for(uint32_t a=0x02000000;a<0x02040000;a++){uint8_t b=c->rawRead8(c,a,-1);fputc(b,f);} fclose(f);
    }
  }
  return 0;
}
