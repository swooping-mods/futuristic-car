/* Original-look car presentation on a separate native saved vehicle identity. */
#define MOONLAB_LATE_UPDATE_ROLE 2
#include "../../runtime/native/gameplay.h"
#include "../vehicle-framework/sdk/client.h"
#include "../vehicle-framework/sdk/static_model.h"
#include "assets/generated/model_data.h"
#include "assets/generated/garage_icon.h"
#include "hover_profile.h"
#include "engine_core.h"
#include "assets/generated/presentation_data.h"
int _fltused=0;
void *memset(void *to,int value,U64 size){unsigned char *p=to;while(size--)*p++=(unsigned char)value;return to;}
static SMResources fc_resources;
static const MoonVehicles *fc_framework;
static PTR fc_log;
static int fc_test;
typedef struct {
    SURoot vehicle,root,transform,renderers[192],joints[4],lamps[4],native_lights,native_rig;
    FCEngine engines[4],published[4];unsigned count;unsigned char enabled[192];
    int remote,native_enabled,native_rig_active,light_state,native_snapshot;
} FCInstance;
static FCInstance fc_instances[32];
static void fc_event(const char *name){if(!fc_log)return;char b[256],*p=cat(b,"{\"event\":\"");p=cat(p,name);p=cat(p,"\"}\n");DWORD n=0;WriteFile(fc_log,b,(DWORD)(p-b),&n,0);}
#include "presentation.h"
static PTR fc_object(PTR component){int ok=1;return call(fc_component_object,component,0,&ok);}
static PTR fc_transform(PTR component){int ok=1;return call(fc_component_transform,component,0,&ok);}
static PTR fc_field(PTR obj,const char *name){PTR f=get_field(object_class(obj),name),value=0;if(f)field_value(obj,f,&value);return value;}
static void fc_remove(FCInstance *v){
    for(unsigned i=0;i<v->count;i++){PTR r=su_get(v->renderers[i]);if(su_alive(r))su_bool(su.renderer_enabled,r,v->enabled[i]);su_drop(v->renderers+i);}
    if((v->native_snapshot&2)&&su_alive(su_get(v->native_rig)))fc_active_set(su_get(v->native_rig),v->native_rig_active);
    if((v->native_snapshot&1)&&su_alive(su_get(v->native_lights)))su_bool(fc_behaviour_set,su_get(v->native_lights),v->native_enabled);
    for(unsigned i=0;i<4;i++){su_drop(v->joints+i);su_drop(v->lamps+i);}
    su_drop(&v->native_lights);su_drop(&v->native_rig);su_drop(&v->transform);
    su_destroy(su_get(v->root));su_drop(&v->root);su_drop(&v->vehicle);su_zero(v,sizeof(*v));
}
static int fc_box(PTR parent,int layer,SPVec3 center,SPVec3 size){
    PTR go=sm_scene_object("Courier fitted hull",parent);if(!go||!sm_layer(go,layer))return 0;PTR box=su_add_component(go,"BoxCollider");if(!box)return 0;
    int ok=1;void *a[]={&center},*b[]={&size};call(su_method("UnityEngine","BoxCollider","set_center","UnityEngine.Vector3"),box,a,&ok);call(su_method("UnityEngine","BoxCollider","set_size","UnityEngine.Vector3"),box,b,&ok);return ok;
}
static int fc_spawn(void *vehicle,int remote){
    U64 begin=fc_clock();
    for(unsigned i=0;i<32;i++)if(su_get(fc_instances[i].vehicle)==vehicle)return 1;
    FCInstance *v=0;for(unsigned i=0;i<32;i++)if(!fc_instances[i].vehicle){v=fc_instances+i;break;}if(!v||!fc_template_prepare())return 0;
    PTR authored=fc_field(vehicle,remote?"<AuthoredView>k__BackingField":"authored");if(!authored)authored=fc_field(vehicle,"<AuthoredView>k__BackingField");if(!su_alive(authored))return 0;
    int ok=1;int layer=integer(su_method("UnityEngine","GameObject","get_layer",""),fc_object(authored),&ok);if(!ok)return 0;
    unsigned char typed=1,recursive=1,inactive=1,reverse=0;void *a[]={su_type("UnityEngine","Renderer"),&typed,&recursive,&inactive,&reverse,0};PTR renderers=call(su_method("UnityEngine","GameObject","GetComponentsInternal","System.Type;System.Boolean;System.Boolean;System.Boolean;System.Boolean;System.Object"),fc_object(authored),a,&ok);
    U64 count=renderers?su.array_length(renderers):0;if(!ok||count>192)return 0;
    v->vehicle=su_hold(vehicle);v->remote=remote;v->light_state=-1;v->transform=su_hold(fc_transform(vehicle));if(!v->vehicle||!v->transform)goto fail;
    /* Snapshot only native renderers before adding our own children. A failure
     * restores their exact enabled states and removes the partial model. */
    SURoot renderer_list=su_hold(renderers);if(!renderer_list)goto fail;
    for(U64 i=0;i<count;i++){
        PTR r=*(PTR *)su.array_address(su_get(renderer_list),8,i);if(!su_alive(r))continue;
        v->enabled[v->count]=(unsigned char)boolean(su_method("UnityEngine","Renderer","get_enabled",""),r,&ok);
        if(!ok){su_drop(&renderer_list);goto fail;}v->renderers[v->count]=su_hold(r);
        if(!v->renderers[v->count]){su_drop(&renderer_list);goto fail;}v->count++;
    }su_drop(&renderer_list);
    /* Native lamps/flares live in a separate rig, outside the authored mesh.
     * Retain their switch state, but suppress that donor presentation exactly. */
    PTR lights=fc_field(vehicle,"lights");
    if(su_alive(lights)){
        v->native_lights=su_hold(lights);v->native_enabled=boolean(fc_behaviour_get,lights,&ok);
        if(!v->native_lights||!ok)goto fail;v->native_snapshot|=1;
        PTR rig=fc_field(lights,"rig");if(su_alive(rig)){
            if(object_class(rig)!=find_class("UnityEngine","GameObject"))rig=fc_object(rig);
            v->native_rig=su_hold(rig);if(!v->native_rig)goto fail;v->native_rig_active=boolean(fc_active,rig,&ok);
            if(!ok)goto fail;v->native_snapshot|=2;
            if(!fc_active_set(rig,0))goto fail;
        }
        if(!v->native_lights||!ok)goto fail;unsigned char disabled=0;void *disable[]={&disabled};call(fc_behaviour_set,lights,disable,&ok);if(!ok)goto fail;
    }
    v->root=fc_take_presentation();PTR root=su_get(v->root);if(!v->root)goto fail;
    PTR root_transform=su_transform(root);unsigned char keep=0;void *parent[]={su_get(v->transform),&keep};call(su.tr_parent,root_transform,parent,&ok);
    su_set_vector(su.tr_local_position,root_transform,su_v(0,.3f,0));su_set_quat(su.tr_local_rotation,root_transform,(SUQuat){0,0,0,1});
    if(!ok||!fc_layer_tree(root,layer))goto fail;
    for(unsigned i=0;i<4;i++){
        char name[64];cat(num(cat(name,"Courier engine "),i),"");v->joints[i]=su_hold(fc_child(root,name));if(!v->joints[i])goto fail;
        const char *names[]={"Courier headlight L","Courier headlight R","Courier taillight L","Courier taillight R"};
        PTR t=fc_child(root,names[i]);void *a[]={fc_light_type};PTR lamp=t?call(fc_component_get,t,a,&ok):0;v->lamps[i]=su_hold(lamp);if(!ok||!v->lamps[i])goto fail;
    }
    v->light_state=v->native_lights?boolean(fc_on,su_get(v->native_lights),&ok):1;
    for(unsigned i=0;i<4;i++)su_bool(fc_behaviour_set,su_get(v->lamps[i]),v->light_state);
    if(!remote)for(unsigned i=0;i<fc_hover_profile.box_count;i++){
        const MoonHoverBox *b=fc_hover_boxes+i;
        if(!fc_box(su_transform(root),layer,su_v(b->center.x,b->center.y-.3f,b->center.z),su_v(b->size.x,b->size.y,b->size.z)))goto fail;
    }
    if(!ok)goto fail;for(unsigned i=0;i<v->count;i++)su_bool(su.renderer_enabled,su_get(v->renderers[i]),0);if(!fc_active_set(root,1))goto fail;fc_event(remote?"remote-courier-created":"courier-created");fc_cost("courier-attach",begin);return 1;
fail:fc_remove(v);fc_event("courier-presentation-failed");return 0;
}
static void fc_destroy(void *vehicle,int remote){(void)remote;for(unsigned i=0;i<32;i++)if(su_get(fc_instances[i].vehicle)==vehicle){fc_remove(fc_instances+i);return;}}
#include "test_tools.h"
#include "trailer_tools.h"
#include "update_profile.h"
static void fc_scene_reset(void){
    su_drop(&fc_test_owned);su_drop(&fc_test_garage);
    for(unsigned i=0;i<32;i++)if(fc_instances[i].vehicle)fc_remove(fc_instances+i);
    for(unsigned i=0;i<2;i++){su_destroy(su_get(fc_spares[i]));su_drop(fc_spares+i);}
    su_destroy(su_get(fc_template));su_drop(&fc_template);su_drop(&fc_world);sm_release(&fc_resources);
    fc_warm_reported=0;fc_replenish_at=0;
}
static int fc_animate(FCInstance *v,float dt){
    MoonHoverState state={0};int moving=!v->remote&&fc_framework->hover_state(su_get(v->vehicle),&state,sizeof(state));
    RVVec force={0};float yaw=0;
    if(moving){
        PTR t=su_get(v->transform);SPVec3 right=su_vector(su.tr_right,t),up=su_vector(su.tr_up,t),forward=su_vector(su.tr_forward,t);
        SPVec3 a=su_v(state.flight.acceleration.x,state.flight.acceleration.y,state.flight.acceleration.z);
        force=rv_v(su_dot(a,right),su_dot(a,up),su_dot(a,forward));yaw=state.flight.yaw_rate;
    }
    for(unsigned i=0;i<4;i++){
        FCEngine *e=v->engines+i,*old=v->published+i;fc_engine_step(e,force,yaw,(int)i,dt);
        if(e->pitch!=old->pitch||e->roll!=old->roll||e->heave!=old->heave){
            SPVec3 up=su_normal(su_v(e->roll,1,e->pitch)),forward=su_normal(su_sub(su_v(0,0,1),su_mul(up,up.z)));
            su_set_quat(su.tr_local_rotation,su_get(v->joints[i]),su_rotation(forward,up));
            su_set_vector(su.tr_local_position,su_get(v->joints[i]),su_add(fc_engine_pivots[i],su_v(0,e->heave,0)));*old=*e;
        }
    }
    int ok=1,on=v->native_lights?boolean(fc_on,su_get(v->native_lights),&ok):1;
    if(ok&&on!=v->light_state){for(unsigned i=0;i<4;i++)su_bool(fc_behaviour_set,su_get(v->lamps[i]),on);v->light_state=on;}
    if(su_alive(su_get(v->native_rig))&&boolean(fc_active,su_get(v->native_rig),&ok))fc_active_set(su_get(v->native_rig),0);
    return moving&&state.output.speed>.5f;
}
static void fc_update(void *walker){
    U64 begin=fc_test&&fct_profile?fc_clock():0;
    fc_test_update(walker);U64 tested=begin?fc_clock():0;
    fc_trailer_update(walker);U64 controlled=begin?fc_clock():0;
    fc_update_profile_sync();
    U64 admitted=0,presented=0;unsigned instances=0;
    PTR world=0;field_value(walker,fc_world_field,&world);
    if(fc_world&&(!su_alive(su_get(fc_world))||su_get(fc_world)!=world))fc_scene_reset();
    if(!su_alive(world))goto done;int ok=1;if(boolean(fc_restoring,world,&ok)||!ok)goto done;
    if(!fc_world){fc_world=su_hold(world);if(!fc_world)goto done;}
    float dt=su_number(fc_time_delta,0);int busy=0;
    admitted=begin?fc_clock():0;
    for(unsigned i=0;i<32;i++)if(fc_instances[i].vehicle){
        instances++;
        if(!su_alive(su_get(fc_instances[i].vehicle)))fc_remove(fc_instances+i);
        else if(dt>0&&dt<=.1f)busy|=fc_animate(fc_instances+i,dt);
    }
    presented=begin?fc_clock():0;
    if(!busy)fc_warm();
done:
    if(begin){U64 ended=fc_clock();if(!admitted)admitted=ended;if(!presented)presented=ended;
        fc_update_profile_sample(begin,tested,controlled,admitted,presented,ended,instances);}
}
static const MoonVehicleDefinition definition={
    .abi=MV_ABI,.size=sizeof(MoonVehicleDefinition),.key="futuristic-car.courier",.id=23,.template_id=4,
    .name="Futuristic Courier",.hint="Cream and orange flying courier. Shift+F9 lift/land; Space/Ctrl altitude; Shift boost.",.summary="Futuristic Courier",.price=1750,.capacity=5,.garage=1,
    .width=3.2f,.depth=4.2f,.height=2.65f,.speed=14,.cockpit=1,.fit_seat=1,.driver={0,1.35f,-.15f},.seat={0,1,-.15f},
    .icon_png=fc_icon_png,.icon_size=sizeof(fc_icon_png)-1,.icon_width=384,.icon_height=384,
    .spawn=fc_spawn,.destroy=fc_destroy,.update=fc_update,.hover=&fc_hover_profile
};
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    (void)num;(void)prepare_hook;(void)install_hooks;
    if(!bind_runtime(domain,game)||!su_bind(game))return 0;fc_framework=mv_connect(domain,game);if(!fc_framework)return 0;
    static char folder[2048],path[2200];DWORD len=GetEnvironmentVariableA("MOONLAB_EVIDENCE",folder,sizeof(folder));if(len&&len<sizeof(folder)){char *p=cat(path,folder);cat(p,"\\futuristic-car.jsonl");fc_log=CreateFileA(path,0x40000000,1,0,2,0,0);}
    char flag[8];fc_test=GetEnvironmentVariableA("MOONLAB_TEST_TOOLS",flag,8)==1&&flag[0]=='1';
    if(!fc_presentation_bind())return 0;
    if(fc_test&&!fc_trailer_bind(game))return 0;
    if(!fc_framework->register_vehicle(&definition))return 0;fc_event("ready");return 1;
}
