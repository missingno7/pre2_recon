#include "native/settings.h"
#include <assert.h>
#undef assert
#define assert(c) do { if(!(c)) {fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#c); exit(1);} } while(0)

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

static void equal(const Pre2Settings *a,const Pre2Settings *b)
{
    for(int id=0;id<SET_COUNT;id++) {
        char av[80],bv[80]; settings_value(a,(Pre2Setting)id,av,sizeof av); settings_value(b,(Pre2Setting)id,bv,sizeof bv);
        assert(!strcmp(av,bv));
    }
}
int main(void)
{
    mkdir("build/overlay_shots");
    const char *path="build/overlay_shots/settings_test.json";
    Pre2Settings defaults=settings_defaults(),s=defaults,loaded;
    PresenterOptions p=settings_presenter(&defaults),q=presenter_defaults();
    assert(p.mode==q.mode && p.aspect==q.aspect && p.camera_tau==q.camera_tau && p.hud==q.hud && p.backdrop==q.backdrop);
    assert(p.interpolate==q.interpolate && p.smooth_camera==q.smooth_camera && p.smooth_transitions==q.smooth_transitions);
    assert(settings_load(&loaded,"build/overlay_shots/absent/no.json",NULL)==1); equal(&loaded,&defaults);
    FILE *partial=fopen(path,"wb"); assert(partial); fputs("{\"volume\":",partial); assert(!fclose(partial));
    assert(settings_load(&loaded,path,NULL)==-1); equal(&loaded,&defaults);
    partial=fopen(path,"wb"); assert(partial); fputs("{\"volume\":17}",partial); assert(!fclose(partial));
    assert(!settings_load(&loaded,path,NULL)); assert(loaded.volume==17 && loaded.vsync==1);
    partial=fopen(path,"wb"); assert(partial); assert(fwrite("{}\0junk",1,7,partial)==7); assert(!fclose(partial));
    assert(settings_load(&loaded,path,NULL)==-1); equal(&loaded,&defaults);
    for(int pass=0;pass<25;pass++) {
        for(int id=0;id<SET_COUNT;id++) settings_adjust(&s,(Pre2Setting)id,1);
        assert(!settings_save(&s,path,stderr)); assert(!settings_load(&loaded,path,stderr)); equal(&s,&loaded);
    }
    s.camera_tau=.12345678901234567;
    assert(!settings_save(&s,path,stderr)); assert(!settings_load(&loaded,path,stderr));
    assert(s.camera_tau==loaded.camera_tau);
    FILE *warnings=fopen("build/overlay_shots/warnings_test.txt","w+b"); assert(warnings);
    for(int id=0;id<SET_COUNT;id++) {
        char json[256]; snprintf(json,sizeof json,"{\"volume\":42,\"%s\":null,\"mute\":true}",settings_key((Pre2Setting)id));
        assert(!settings_parse(&loaded,json,warnings));
        char a[80],b[80]; settings_value(&loaded,(Pre2Setting)id,a,sizeof a);
        settings_value(&defaults,(Pre2Setting)id,b,sizeof b);
        if(id!=SET_MUTE) assert(!strcmp(a,b));
        assert(loaded.mute); if(id!=SET_VOLUME) assert(loaded.volume==42);
    }
    assert(ftell(warnings)>0); fclose(warnings); remove("build/overlay_shots/warnings_test.txt");
    const char *bad[]={"", "[]", "{", "{\"mute\":true,}", "{\"volume\":01}",
        "{\"volume\":1.}","{\"volume\":1e}","{\"x\": [1,]}","{} trailing",
        "{\"x\":\"\\q\"}","{\"x\":\"\\", "{\"x\":\"\\u12\"}","{\"x\":truefalse}","{\"version\":2}"};
    for(size_t i=0;i<sizeof bad/sizeof *bad;i++) {assert(settings_parse(&loaded,bad[i],NULL)==-1); equal(&loaded,&defaults);}
    assert(!settings_parse(&loaded,"{\"mode\":\"enhanced\",\"mystery\":{\"a\":[true,null,\"hi\\n\",-1.2e+2]},\"volume\":27}",NULL));
    assert(loaded.mode==1 && loaded.volume==27 && loaded.vsync==1);
    assert(!settings_parse(&loaded,"{\"\\u006dode\":\"widescreen\",\"wide_sim\":\"auto\"}",NULL));
    assert(loaded.mode==2 && loaded.wide_sim==-1);
    assert(!settings_parse(&loaded,"{\"camera_tau\":0,\"volume\":101,\"hud\":\"none\",\"wide_sim\":333,\"mute\":1,\"vsync\":\"false\",\"mode\":2}",NULL));
    equal(&loaded,&defaults);
    assert(!settings_parse(&loaded,"{\"camera_tau\":1e999,\"frame_cap\":-1,\"window_scale\":2.5}",NULL)); equal(&loaded,&defaults);
    assert(!settings_parse(&loaded,"{\"wide_sim\":640,\"pixel_aspect\":\"crt\",\"hud\":\"anchored\"}",NULL));
    assert(loaded.wide_sim==640 && loaded.pixel_aspect==1);
    /* Defaults < file < CLI. Only the edited field is copied to persistence. */
    Pre2Settings persisted=loaded,runtime=loaded;
    assert(!settings_set(&runtime,SET_VOLUME,"11")); assert(!settings_set(&runtime,SET_MODE,"enhanced"));
    settings_adjust(&runtime,SET_MUTE,1); settings_copy(&persisted,&runtime,SET_MUTE);
    assert(persisted.mute && persisted.volume==100 && persisted.mode==0);
    assert(!settings_save(&persisted,path,stderr)); assert(!settings_load(&loaded,path,stderr)); equal(&persisted,&loaded);
    assert(settings_save(&runtime,"build/overlay_shots/absent/settings.json",NULL)==-1);
    assert(!settings_load(&loaded,path,NULL)); equal(&persisted,&loaded);
    /* Replacement must leave no .tmp behind, and a second save replaces it. */
    FILE *temp=fopen("build/overlay_shots/settings_test.json.tmp","rb"); assert(!temp);
    remove(path);
    puts("settings_json: round trips, strict syntax, partial/unknown fields, per-key fallback, warnings, precedence and atomic replacement pass");
    return 0;
}
