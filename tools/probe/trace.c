#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/core/log.h>
#include <mgba/internal/arm/arm.h>
#include <mgba/internal/gba/gba.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void nolog(struct mLogger* l, int c, enum mLogLevel lv, const char* f, va_list a){}
static struct mLogger L = { .log = nolog };
static uint8_t romhit[0x2000000/2]; // bit0 thumb bit1 arm, per halfword
static uint32_t shadow[4096]; static int sp_=0;
static uint32_t ent[1<<20]; static int nent=0;
static uint8_t ramhit_iw[0x8000/2], ramhit_ew[0x40000/2];
int main(int argc,char**argv){
  mLogSetDefaultLogger(&L);
  struct mCore* c = mCoreFind(argv[1]); c->init(c); mCoreInitConfig(c,NULL);
  uint32_t* fb = malloc(240*160*4); c->setVideoBuffer(c,(void*)fb,240);
  mCoreLoadFile(c,argv[1]); c->reset(c);
  int frames=atoi(argv[2]);
  int nin=0; static int inf[100000], ink[100000];
  FILE*f=fopen(argv[3],"r"); while(nin<100000&&fscanf(f,"%d %i",&inf[nin],&ink[nin])==2) nin++; fclose(f);
  struct ARMCore* cpu = c->cpu;
  int ii=0,keys=0;
  const char* shotdir=getenv("SHOTDIR");
  for(int fr=0;fr<frames;fr++){
    while(ii<nin && inf[ii]<=fr){keys=ink[ii];ii++;}
    c->setKeys(c,keys);
    if(shotdir && fr%5000==0){char p[512];snprintf(p,sizeof p,"%s/s%07d.rgba",shotdir,fr);FILE*g=fopen(p,"wb");fwrite(fb,4,240*160,g);fclose(g);}
    uint32_t start=c->frameCounter(c);
    while(c->frameCounter(c)==start){
      int t = cpu->executionMode==MODE_THUMB;
      uint32_t pc = cpu->gprs[15] - (t?2:4);
      uint8_t bit = t?1:2;
      if(pc>=0x08000000 && pc<0x0A000000) romhit[(pc-0x08000000)>>1]|=bit;
      else if(pc>=0x03000000 && pc<0x03008000) ramhit_iw[(pc-0x03000000)>>1]|=bit;
      else if(pc>=0x02000000 && pc<0x02040000) ramhit_ew[(pc-0x02000000)>>1]|=bit;
      uint32_t op = t ? c->rawRead16(c,pc,-1) : c->rawRead32(c,pc,-1);
      int kind=0; // 1=bl 2=bx
      if(t){ if((op&0xF800)==0xF800) kind=1; else if((op&0xFF87)==0x4700 && ((op>>3)&15)!=14) kind=2; }
      else { uint32_t cond=op>>28; if(cond==14 && (op&0x0F000000)==0x0B000000) kind=1; else if(cond==14 && (op&0x0FFFFFF0)==0x012FFF10 && (op&15)!=14) kind=2; }
      c->step(c);
      if(kind){
        int t2 = cpu->executionMode==MODE_THUMB;
        uint32_t npc = cpu->gprs[15] - (t2?2:4);
        if(kind==1){ if(sp_<4096) shadow[sp_++]= (pc+(t?2:4)) & ~1u; }
        else {
          int found=-1; for(int k=sp_-1;k>=0 && k>=sp_-64;k--) if(shadow[k]==npc){found=k;break;}
          if(found>=0) sp_=found;
          else if(nent < (1<<20)) ent[nent++] = npc | (t2?1:0);
        }
        // also record BL targets so callers can compare (not needed)
      }
    }
  }
  f=fopen(argv[4],"wb"); fwrite(romhit,1,sizeof romhit,f); fwrite(ramhit_iw,1,sizeof ramhit_iw,f); fwrite(ramhit_ew,1,sizeof ramhit_ew,f); fclose(f);
  { const char*ef=getenv("ENTRIES"); FILE*g=fopen(ef?ef:"entries.txt","a"); for(int k=0;k<nent;k++) fprintf(g,"%08X\n",ent[k]); fclose(g);}
  // dump final ram
  f=fopen(argv[5],"wb"); for(uint32_t a=0x03000000;a<0x03008000;a++) fputc(c->rawRead8(c,a,-1),f); for(uint32_t a=0x02000000;a<0x02040000;a++) fputc(c->rawRead8(c,a,-1),f); fclose(f);
  f=fopen("last.rgba","wb"); fwrite(fb,4,240*160,f); fclose(f);
  return 0;
}
