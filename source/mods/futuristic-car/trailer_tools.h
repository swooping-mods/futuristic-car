/* Disposable harness only: film real vehicle physics with a leased camera.
 * F2 advances moves; F3 restores camera, HUD and pacing. F5 prewarms all views.
 * F11 buffers/dumps cadence; F4 toggles the old lookup path for attribution.
 * Ctrl+F4 compares software 60 fps pacing with display VSync pacing.
 * No capture controls are installed in ordinary player/loader sessions. */
#include "../../runtime/native/camera.h"
#include "trailer_state.h"
typedef struct {U64 interval,cost;unsigned frame,shot,cached,phase;} FCTFrame;
static FCTFrame fct_frames[4096];
static unsigned fct_frame_count,fct_profile,fct_overflow;
static U64 fct_frequency,fct_previous;
static BOOL (*fct_counter)(U64 *);
static PTR fct_log;
static U64 fct_clock(void){U64 now=0;if(fct_counter)fct_counter(&now);return now;}
static void fct_event(const char *event){
    char b[256],*p=cat(b,"{\"event\":\"");p=cat(p,event);p=cat(p,"\"}\n");DWORD n=0;
    if(fct_log){WriteFile(fct_log,b,(DWORD)(p-b),&n,0);FlushFileBuffers(fct_log);}
}
static void fct_dump(void){
    if(!fct_frame_count)return;
    char b[512];DWORD n=0;
    for(unsigned i=0;i<fct_frame_count;i++){
        FCTFrame *r=fct_frames+i;char *p=cat(b,"{\"event\":\"frame\",\"interval_us\":");p=num(p,r->interval*1000000/fct_frequency);
        p=cat(p,",\"camera_us\":");p=num(p,r->cost*1000000/fct_frequency);p=cat(p,",\"unity_frame\":");p=num(p,r->frame);
        p=cat(p,",\"shot\":");p=num(p,r->shot);p=cat(p,",\"cached\":");p=num(p,r->cached);p=cat(p,",\"phase\":");p=num(p,r->phase);p=cat(p,"}\n");
        WriteFile(fct_log,b,(DWORD)(p-b),&n,0);
    }
    FlushFileBuffers(fct_log);fct_frame_count=0;fct_previous=0;
    fct_event(fct_overflow?"profile-overflow":"profile-complete");fct_overflow=0;
}
typedef struct {float angle,arc,distance,end_distance,height,end_height,fov;} FCTShot;
static const FCTShot fct_shots[]={
    {2.5f,-.8f,8,7,2.5f,2.1f,42},
    {-.75f,.55f,7,6,2,1.7f,42},
    {1.25f,.3f,4.8f,5.2f,1,1.5f,46},
    {2.8f,-.12f,12,11,4.6f,4,48},
    {-1.35f,.24f,17,15,6,5,46},
    {-.7f,.4f,8,9.5f,2.4f,3.2f,42}
};
static float fct_sin(float x){
    while(x>3.14159265f)x-=6.2831853f;while(x< -3.14159265f)x+=6.2831853f;
    float q=x*x;return x*(1-q/6+q*q/120-q*q*q/5040+q*q*q*q/362880-q*q*q*q*q/39916800);
}
static PTR fct_original_transform(PTR component){
    int ok=1;return call(su_method("UnityEngine","Component","get_transform",""),component,0,&ok);
}
static int fct_pacing(unsigned hardware_sync){
    int ok=1,target=hardware_sync?-1:60,vsync=hardware_sync?1:0;
    void *a[]={&target},*b[]={&vsync};
    call(fct.set_target,0,a,&ok);call(fct.set_vsync,0,b,&ok);
    if(!ok||integer(fct.get_target,0,&ok)!=target||integer(fct.get_vsync,0,&ok)!=vsync||!ok){fct_event("pacing-failed");return 0;}
    fct.hardware_sync=hardware_sync;fct_event(hardware_sync?"pacing-vsync":"pacing-software");return 1;
}
static int fct_control_down(void){
    static short (*state)(int);
    if(!state)state=(short(*)(int))GetProcAddress(LoadLibraryA("user32.dll"),"GetAsyncKeyState");
    return state&&(state(0x11)&0x8000)!=0;
}
#include "trailer_lifetime.h"
static void fct_hide_hud(void){
    int ok=1,inactive=0,sort=0;void *a[]={su_type("UnityEngine","Canvas"),&inactive,&sort};
    PTR array=call(su_method("UnityEngine","Object","FindObjectsByType","System.Type;UnityEngine.FindObjectsInactive;UnityEngine.FindObjectsSortMode"),0,a,&ok);
    U64 count=array?su.array_length(array):0;
    for(U64 i=0;ok&&i<count&&fct.count<128;i++){
        PTR canvas=*(PTR *)su.array_address(array,8,i);int known=0;
        for(unsigned j=0;j<fct.count;j++)if(su_get(fct.canvases[j])==canvas)known=1;
        if(known)continue;int enabled=boolean(fct.canvas_enabled,canvas,&ok);if(!ok)break;
        SURoot root=su_hold(canvas);if(!root)break;
        fct.canvases[fct.count]=root;fct.enabled[fct.count++]=(unsigned char)enabled;
        su_bool(fct.canvas_set,canvas,0);
    }
}
static int fct_before(int id,Context *context){
    (void)id;PTR walker=(PTR)context->rcx;if(!fct.session.token||!fct.captured||fct.returning||walker!=su_get(fct.walker))return 0;
    U64 entered=fct_profile?fct_clock():0;PTR world=0,camera=0;
    field_value(walker,fct.world_field,&world);field_value(walker,fct.camera_field,&camera);
    if(world!=su_get(fct.world)||camera!=su_get(fct.camera)||!su_alive(su_get(fct.vehicle))||!su_alive(su_get(fct.camera_tr))||!su_alive(su_get(fct.vehicle_tr))){fct_stop();return 0;}
    if(!fct.shot)return 0;
    if(fct.warming){
        ++fct.warm_frame;
        if(fct.warm_frame<360){
            unsigned shot=fct.warm_frame/60+1;
            if(shot!=fct.shot){fct.shot=shot;su_float(fct.set_fov,su_get(fct.camera),fct_shots[shot-1].fov);}
        }else if(fct.warm_frame==360){fct.shot=fct.warm_shot;fct.start=su_number(su.time_unscaled,0);su_float(fct.set_fov,su_get(fct.camera),fct_shots[fct.shot-1].fov);}
        else if(fct.warm_frame>=480){fct.warming=0;fct.start=su_number(su.time_unscaled,0);fct_event("warmup-complete");}
    }
    const FCTShot *s=fct_shots+fct.shot-1;
    float t=(su_number(su.time_unscaled,0)-fct.start)/12;if(t<0)t=0;if(t>1)t=1;t=t*t*(3-2*t);
    float angle=s->angle+s->arc*t,distance=s->distance+(s->end_distance-s->distance)*t,height=s->height+(s->end_height-s->height)*t;
    /* Rooted transforms and prebound setters avoid metadata enumeration and
     * redundant lens writes in the recording path. The old branch is kept
     * only for a matched, same-process attribution measurement. */
    PTR vt=fct.cached?su_get(fct.vehicle_tr):fct_original_transform(su_get(fct.vehicle)),ct=fct.cached?su_get(fct.camera_tr):fct_original_transform(su_get(fct.camera));
    SPVec3 p=su_vector(su.tr_get_position,vt),up=su_normal(p),forward=su_vector(su.tr_forward,vt);
    /* Gravity, rather than the banked hull, defines the horizon. */
    forward=su_normal(su_sub(forward,su_mul(up,su_dot(forward,up))));SPVec3 right=su_normal(su_cross(up,forward));
    SPVec3 offset=su_add(su_mul(forward,fct_sin(angle+1.5707963f)*distance),su_mul(right,fct_sin(angle)*distance));
    SPVec3 eye=su_add(p,su_add(offset,su_mul(up,height))),target=su_add(p,su_mul(up,1.15f));
    su_set_vector(su.tr_position,ct,eye);su_set_quat(su.tr_rotation,ct,su_rotation(su_normal(su_sub(target,eye)),up));
    if(!fct.cached)su_float(fct.set_fov,su_get(fct.camera),s->fov);
    if(fct_profile){
        U64 ended=fct_clock();int ok=1;unsigned frame=(unsigned)integer(fct.frame,0,&ok);
        if(ok&&fct_frame_count<4096){fct_frames[fct_frame_count++]=(FCTFrame){fct_previous?entered-fct_previous:0,ended-entered,frame,fct.shot,fct.cached,fct.warming?(fct.warm_frame<360?1u:2u):0u};}
        else fct_overflow=1;
        fct_previous=entered;
    }
    return 1;
}
#include "trailer_controls.h"
static int fc_trailer_bind(PTR game){
    fct.api=moon_camera_connect();
    fct.get_fov=su_method("UnityEngine","Camera","get_fieldOfView","");fct.set_fov=su_method("UnityEngine","Camera","set_fieldOfView","System.Single");
    fct.canvas_enabled=su_method("UnityEngine","Behaviour","get_enabled","");fct.canvas_set=su_method("UnityEngine","Behaviour","set_enabled","System.Boolean");
    fct.component_transform=su_method("UnityEngine","Component","get_transform","");fct.frame=su_method("UnityEngine","Time","get_frameCount","");
    fct.get_target=su_method("UnityEngine","Application","get_targetFrameRate","");fct.set_target=su_method("UnityEngine","Application","set_targetFrameRate","System.Int32");
    fct.get_vsync=su_method("UnityEngine","QualitySettings","get_vSyncCount","");fct.set_vsync=su_method("UnityEngine","QualitySettings","set_vSyncCount","System.Int32");
    fct.world_field=get_field(find_class("MoonCleaners","MoonWalker"),"world");fct.camera_field=get_field(find_class("MoonCleaners","MoonWalker"),"viewCamera");fct.cached=1;
    PTR kernel=GetModuleHandleA("kernel32.dll");fct_counter=(BOOL(*)(U64 *))GetProcAddress(kernel,"QueryPerformanceCounter");BOOL (*frequency)(U64 *)=(BOOL(*)(U64 *))GetProcAddress(kernel,"QueryPerformanceFrequency");
    static char folder[2048],path[2200];DWORD n=GetEnvironmentVariableA("MOONLAB_EVIDENCE",folder,sizeof(folder));
    if(n&&n<sizeof(folder)){cat(cat(path,folder),"\\courier-trailer.jsonl");fct_log=CreateFileA(path,0x40000000,1,0,2,0,0);}
    if(!fct.api||!fct.get_fov||!fct.set_fov||!fct.canvas_enabled||!fct.canvas_set||!fct.component_transform||!fct.frame||!fct.get_target||!fct.set_target||!fct.get_vsync||!fct.set_vsync||!fct.world_field||!fct.camera_field||!fct_counter||!frequency||!frequency(&fct_frequency)||!fct_frequency||!fct_log||fct_log==(PTR)(U64)-1)return 0;
    fct_event(su_method("UnityEngine","ScreenCapture","CaptureScreenshot","System.String;System.Int32")?"offline-screenshot-api-present":"offline-screenshot-api-stripped");
    if(!prepare_hook(method("MoonCleaners","MoonWalker","LateUpdate",0),100671741,0,game))return 0;
    before_hook=fct_before;return install_hooks();
}
