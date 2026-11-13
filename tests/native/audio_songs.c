/* Bounded constructed music runs: no ProgramEntry, no interactive execution. */
#include "native/audio_internal.h"
#undef DB
#undef DW
#undef SET_DB
#undef SET_DW
#undef rd8
#undef rd16
#undef wr8
#undef wr16
#include "script.h"
#include "c/startup.h"
#include "c/system.h"
#include "c/sound.h"
#include "c/interrupts.h"
#include "c/platform.h"
#include "c/ports.h"
static Audio *engine;
static uint64_t clock_ns;
static uint8_t ref_block[168],native_block[168];
static unsigned ref_count,native_count;
static int cmd,argc_dsp,output_error;
static void match_output(void) {
    if (ref_count==native_count && memcmp(ref_block,native_block,168)) {
        size_t k=0; while (k<168 && ref_block[k]==native_block[k]) k++;
        if (!output_error) printf("PCM sequence mismatch block %u byte %zu %02x/%02x at %llu\n",
            ref_count-1,k,ref_block[k],native_block[k],(unsigned long long)clock_ns);
        output_error=1;
    }
}
static void out(uint16_t port,uint8_t v) {
    if (port!=0x22c) return;
    if (cmd<0) { if (v==0x14 || v==0x40) { cmd=v; argc_dsp=0; } }
    else if (++argc_dsp==(cmd==0x14?2:1)) {
        if (cmd==0x14) { memcpy(ref_block,pre2_mem+lin(DATA_SEG,DW(DS_PlayBuffer)),168); ref_count++; match_output(); }
        cmd=-1;
    }
}
static uint8_t in(uint16_t p) { return p==0x22e?0xff:0; }
static PortDevice ports={out,in};
static void pcm(void *user,uint64_t t,const uint8_t *p) {
    (void)user;
    if (t!=clock_ns) { printf("bad timestamp %llu/%llu\n",(unsigned long long)t,(unsigned long long)clock_ns); exit(1); }
    memcpy(native_block,p,168); native_count++; match_output();
}
static void sink(const AudioCmd *c) { if (audio_command(engine,c,clock_ns)) { printf("bad command %u/%u\n",c->kind,c->id); exit(1); } }
static void trigger(unsigned id) { audio_source(AUDIO_SRC_NONE,0,0,0); StartSoundEffect((uint16_t)id); }
static void toggle(void) {
    SET_CB(CS_SoundState,CB(CS_SoundState)^1);
    audio_emit(AUDIO_OUTPUT_TOGGLE,CB(CS_SoundState)&1);
    if (CB(CS_SoundState)&1) StartSbBlock();
}
static int check(unsigned slot,unsigned block) {
    if (output_error || ref_count!=native_count || memcmp(ref_block,native_block,168)) {
        size_t k=0; while (k<168 && ref_block[k]==native_block[k]) k++;
        printf("slot %u block %u: first mismatch byte %zu %02x/%02x, counts %u/%u\n",slot,block,k,k<168?ref_block[k]:0,k<168?native_block[k]:0,ref_count,native_count);
        printf(" order %x/%x row %x/%x length %x/%x patterns %x/%x\n",DW(DS_SongOrderPos),aw(engine,DS_SongOrderPos),DW(DS_SongRow),aw(engine,DS_SongRow),DB(DS_SongLength),ab(engine,DS_SongLength),DB(DS_PatternCount),ab(engine,DS_PatternCount));
        for (unsigned ch=0;ch<8;ch+=2) {
            uint16_t rec=(uint16_t)(DW(DS_ChannelInstrument+ch)<<4);
            uint16_t nr=(uint16_t)(aw(engine,DS_ChannelInstrument+ch)<<4);
            printf(" ch %u pos %x/%x end %x/%x vol %x/%x step %x/%x phase %x/%x inst %x/%x len %x/%x\n",ch,
                DW(DS_ChannelPos+ch),aw(engine,DS_ChannelPos+ch),DW(DS_ChannelEnd+ch),aw(engine,DS_ChannelEnd+ch),
                DW(DS_ChannelVolume+ch),aw(engine,DS_ChannelVolume+ch),DW(DS_ChannelStep+ch),aw(engine,DS_ChannelStep+ch),
                DW(DS_ChannelPhase+ch),aw(engine,DS_ChannelPhase+ch),rec,nr,DW(DS_InstrumentTable+rec),aw(engine,DS_InstrumentTable+nr));
        }
        return 1;
    }
    return 0;
}
static int images(unsigned slot) {
    uint16_t m=DW(DS_ModuleSegment);
    unsigned differences=0;
    for (size_t i=0;i<engine->songs[slot].n;i++) if (pre2_mem[lin(m,0)+i]!=engine->module[i]) {
        if (differences++<6) printf(" module mismatch at %zx %02x/%02x\n",i,pre2_mem[lin(m,0)+i],engine->module[i]);
    }
    return differences!=0;
}
int main(int argc,char **argv) {
    static const unsigned slots[]={0,1,2,3,4,9,10,13,14,15,16,17};
    size_t total=0;
    for (size_t song=0;song<12;song++) {
        unsigned slot=slots[song];
        rt_boot(arg_assets(argc,argv),1995,6,1); pre2_frames_run=0;
        pre2_host_poll=NULL; pre2_host_io=NULL;
        ports_install(&ports); cmd=-1; clock_ns=0; ref_count=native_count=0; output_error=0;
        memset(ref_block,0,168); memset(native_block,0,168);
        engine=audio_create(pcm,NULL); if (!engine) return 1;
        pre2_audio_sink=sink;
        SET_CW(CS_ConventionalTopSeg,0xa000);
        SET_DW(DS_NextFreeSegment,(0xb1bcu>>4)+DATA_SEG+1);
        int status=InitSoundDevices(); if (!status) status=InitResourceMemory(); if (!status) status=ActivateResourceSlot((uint16_t)slot);
        if (status) { printf("slot %u setup status %d\n",slot,status); return 1; }
        if (images(slot) || check(slot,0)) return 1;
        size_t bank_bytes=(size_t)DW(DS_SfxBankParagraphs)*16;
        if (memcmp(pre2_mem+lin(DW(DS_SfxBankSegment),0),engine->sample,bank_bytes)) {
            puts("converted SAMPLE allocation mismatch"); return 1;
        }

        /* First pass is clean: cover the full order list, including the original's
         * order==SongLength extra entry, and its loop. Effects may change speed. */
        uint8_t seen[256]={0};
        unsigned blocks=0,loops=0; uint16_t previous=DW(DS_SongOrderPos);
        while (blocks<100000 && loops<2) {
            clock_ns=audio_deadline(engine);
            if (clock_ns==UINT64_MAX) { puts("unexpected stopped chain"); return 1; }
            if (DB(DS_SongTickCount)==1) seen[(uint8_t)DW(DS_SongOrderPos)]=1;
            SbIrqHandler(0x1f0); audio_complete(engine); blocks++;
            if (check(slot,blocks)) return 1;
            uint16_t pos=DW(DS_SongOrderPos);
            if (pos<previous) loops++;
            previous=pos;
        }
        if (loops<2) { printf("slot %u failed to loop\n",slot); return 1; }
        unsigned covered=0;
        for (unsigned order=0;order<DB(DS_SongLength);order++) {
            if (!seen[order]) { printf("slot %u missed order %u\n",slot,order); return 1; }
            covered++;
        }
        unsigned stressed=0;
        /* Interleave SFX at differing offsets, identical retriggers, off/on, and
         * reload a different song while SFX remains. No premix on any restart. */
        for (unsigned i=0;i<2000;i++) {
            uint64_t end=audio_deadline(engine);
            if (end==UINT64_MAX) end=clock_ns+AUDIO_BLOCK_NS;
            clock_ns=end-AUDIO_BLOCK_NS+(i*7919u)%AUDIO_BLOCK_NS;
            if (i%31==0) { trigger(i%11); trigger(i%11); trigger((i+3)%11); }
            if (i%137==0) toggle();
            if (i%137==3) toggle();
            if (i==999) { trigger(7); int st=ActivateResourceSlot((uint16_t)slots[(song+1)%12]); if (!st && images(slots[(song+1)%12])) return 1; if (st) { printf("change status %d\n",st); return 1; } }
            if (audio_deadline(engine)!=UINT64_MAX) {
                clock_ns=audio_deadline(engine); SbIrqHandler(0x1f0); audio_complete(engine); stressed++;
                if (check(slot,blocks+i+1)) return 1;
            }
        }
        total+=ref_count;
        printf("slot %2u: %u orders, %u loops, %u clean + %u stress completions, %u PCM blocks\n",
            slot,covered,loops,blocks,stressed,ref_count);
        audio_destroy(engine); engine=NULL; pre2_audio_sink=NULL;
    }
    /* /B still starts a chain, marks readiness, and copies SFX, while all four
     * music voices and the sequencer remain frozen. No SONG_END is emitted. */
    rt_boot(arg_assets(argc,argv),1995,6,1);
    pre2_host_poll=NULL; pre2_host_io=NULL; ports_install(&ports);
    cmd=-1; clock_ns=0; ref_count=native_count=0; output_error=0;
    memset(ref_block,0,168); memset(native_block,0,168);
    engine=audio_create(pcm,NULL); if (!engine) return 1;
    pre2_audio_sink=sink;
    SET_CW(CS_OptionBits,CW(CS_OptionBits)|0x40);
    SET_CW(CS_ConventionalTopSeg,0xa000);
    SET_DW(DS_NextFreeSegment,(0xb1bcu>>4)+DATA_SEG+1);
    if (InitSoundDevices() || ActivateResourceSlot(0)) return 1;
    for (unsigned i=0;i<1000;i++) {
        clock_ns=audio_deadline(engine)-1;
        if (i%17==0) { trigger(i%11); trigger(i%11); }
        clock_ns=audio_deadline(engine); SbIrqHandler(0x1f0); audio_complete(engine);
        if (check(0,i)) return 1;
    }
    printf("/B: %u PCM blocks, SFX only, byte-equal\n",ref_count);
    total+=ref_count; audio_destroy(engine); engine=NULL; pre2_audio_sink=NULL;
    printf("audio_songs: 12 songs, %zu PCM blocks; byte-equal\n",total);
    return 0;
}
