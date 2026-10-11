#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/core/log.h>
#include <mgba/core/blip_buf.h>
#include <stdio.h>
#include <stdlib.h>
static void nolog(struct mLogger* l, int c, enum mLogLevel lv, const char* f, va_list a){}
static struct mLogger L = { .log = nolog };
int main(int argc,char**argv){
  mLogSetDefaultLogger(&L);
  struct mCore* c = mCoreFind(argv[1]); c->init(c); mCoreInitConfig(c,NULL);
  uint32_t* fb = malloc(240*160*4); c->setVideoBuffer(c,(void*)fb,240);
  mCoreLoadFile(c,argv[1]); c->reset(c);
  int frames=atoi(argv[2]);
  int nin=0; static int inf[100000], ink[100000];
  FILE*f=fopen(argv[3],"r"); while(nin<100000&&fscanf(f,"%d %i",&inf[nin],&ink[nin])==2) nin++; fclose(f);
  FILE*o=fopen(argv[4],"wb");
  blip_t* l=c->getAudioChannel(c,0); blip_t* r=c->getAudioChannel(c,1);
  double clock=c->frequency(c);
  blip_set_rates(l,clock,48000); blip_set_rates(r,clock,48000);
  int ii=0,keys=0; static short bl[8192],br[8192],st[16384];
  for(int fr=0;fr<frames;fr++){
    while(ii<nin && inf[ii]<=fr){keys=ink[ii];ii++;}
    c->setKeys(c,keys); c->runFrame(c);
    int n=blip_samples_avail(l); if(n>8192)n=8192;
    blip_read_samples(l,bl,n,0); blip_read_samples(r,br,n,0);
    for(int i=0;i<n;i++){st[2*i]=bl[i];st[2*i+1]=br[i];}
    fwrite(st,4,n,o);
  }
  fclose(o); return 0;
}
