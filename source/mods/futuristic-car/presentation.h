/* Courier-owned immutable template and two bounded inactive spare instances.
 * Shared static-model/PBR APIs stay unchanged for independent prop consumers. */
static SURoot fc_template,fc_spares[2],fc_world;
static int fc_warm_reported;
static float fc_replenish_at;
static PTR fc_component_object,fc_component_transform,fc_get_components,fc_transform_type;
static PTR fc_active,fc_set_active,fc_clone,fc_find,fc_component_get,fc_light_type;
static PTR fc_behaviour_get,fc_behaviour_set,fc_on,fc_time_delta,fc_restoring,fc_world_field;
static PTR fc_light_kind,fc_light_range,fc_light_angle,fc_light_intensity,fc_light_colour;
static U64 fc_frequency;
static int (*fc_counter)(U64 *);
static U64 fc_clock(void){U64 now=0;if(fc_counter)fc_counter(&now);return now;}
static void fc_cost(const char *name,U64 begin){
    if(!fc_test||!fc_log||!fc_frequency)return;
    char b[256],*p=cat(b,"{\"event\":\"");p=cat(p,name);p=cat(p,"\",\"us\":");p=num(p,(fc_clock()-begin)*1000000/fc_frequency);p=cat(p,"}\n");DWORD n=0;WriteFile(fc_log,b,(DWORD)(p-b),&n,0);
}
static int fc_active_set(PTR go,int active){int ok=1;unsigned char value=(unsigned char)active;void *a[]={&value};call(fc_set_active,go,a,&ok);return ok;}
static PTR fc_components(PTR go,PTR type,int *ok){
    unsigned char typed=1,recursive=1,inactive=1,reverse=0;void *a[]={type,&typed,&recursive,&inactive,&reverse,0};return call(fc_get_components,go,a,ok);
}
static int fc_layer_tree(PTR go,int layer){
    int ok=1;PTR array=fc_components(go,fc_transform_type,&ok);U64 count=array?su.array_length(array):0;
    if(!ok||!count||count>128)return 0;
    SURoot hold=su_hold(array);if(!hold)return 0;
    for(U64 i=0;i<count;i++){
        PTR t=*(PTR *)su.array_address(su_get(hold),8,i),child=call(fc_component_object,t,0,&ok);
        if(!ok||!sm_layer(child,layer)){su_drop(&hold);return 0;}
    }su_drop(&hold);return 1;
}
static PTR fc_child(PTR root,const char *name){int ok=1;void *a[]={su.string(name)};PTR t=call(fc_find,su_transform(root),a,&ok);return ok?t:0;}
static int fc_light_make(PTR parent,SPVec3 position,const char *name,int kind){
    PTR go=sm_scene_object(name,parent);if(!go)return 0;PTR light=su_add_component(go,"Light");if(!light)return 0;
    PTR t=su_transform(go);su_set_vector(su.tr_local_position,t,position);
    SPVec3 aim=kind==0?su_v(0,-.04f,1):kind==1?su_v(0,-.10f,-1):su_v(0,-1,0);
    su_set_quat(su.tr_local_rotation,t,su_rotation(aim,kind==2?su_v(0,0,1):su_v(0,1,0)));
    su_int(fc_light_kind,light,0);su_float(fc_light_range,light,kind==0?15:kind==1?1.6f:1.4f);
    su_float(fc_light_angle,light,kind==0?48:72);su_float(fc_light_intensity,light,kind==0?1.3f:kind==1?.18f:1.1f);
    SUColor c=kind==0?(SUColor){.78f,.9f,1,1}:kind==1?(SUColor){1,.015f,.005f,1}:(SUColor){.01f,.7f,1,1};
    int ok=1;void *a[]={&c};call(fc_light_colour,light,a,&ok);return ok;
}
static int fc_template_prepare(void){
    if(su_alive(su_get(fc_template)))return 1;
    U64 begin=fc_clock();if(!sm_prepare(&fc_model,&fc_resources))return 0;
    PTR go=sm_attach(&fc_model,&fc_resources,0,0,"Courier prepared presentation");fc_template=su_hold(go);
    if(!fc_template||!fc_active_set(go,0))goto fail;
    PTR root=su_transform(go),joints[4];
    for(unsigned i=0;i<4;i++){
        char name[64];cat(num(cat(name,"Courier engine "),i),"");PTR joint=sm_scene_object(name,root);if(!joint)goto fail;
        joints[i]=su_transform(joint);su_set_vector(su.tr_local_position,joints[i],fc_engine_pivots[i]);
        if(!fc_light_make(joints[i],su_sub(fc_engine_emitters[i],fc_engine_pivots[i]),"Courier engine downlight",2))goto fail;
    }
    for(unsigned i=0;i<fc_model.part_count;i++)if(fc_part_engine[i]>=0){
        PTR part=fc_child(go,fc_model.parts[i].name);if(!part)goto fail;int engine=fc_part_engine[i];
        int ok=1;unsigned char keep=0;void *a[]={joints[engine],&keep};call(su.tr_parent,part,a,&ok);if(!ok)goto fail;
        su_set_vector(su.tr_local_position,part,su_mul(fc_engine_pivots[engine],-1));
    }
    for(unsigned i=0;i<2;i++){
        if(!fc_light_make(root,fc_headlight_positions[i],i?"Courier headlight R":"Courier headlight L",0)||
           !fc_light_make(root,fc_taillight_positions[i],i?"Courier taillight R":"Courier taillight L",1))goto fail;
    }
    /* Submit static buffers ahead of selection. Shader/renderer readiness is
     * qualified by real visible instances, not claimed from an inactive tree. */
    PTR upload=su_method("UnityEngine","Mesh","UploadMeshData","System.Boolean");
    if(upload)for(unsigned i=0;i<fc_resources.count;i++){int ok=1;unsigned char unreadable=0;void *a[]={&unreadable};call(upload,su_get(fc_resources.meshes[i]),a,&ok);if(!ok)goto fail;}
    fc_cost("courier-template-prepared",begin);return 1;
fail:su_destroy(su_get(fc_template));su_drop(&fc_template);return 0;
}
static SURoot fc_clone_presentation(void){
    if(!fc_template_prepare())return 0;int ok=1;PTR original=su_get(fc_template);void *a[]={original};PTR clone=call(fc_clone,0,a,&ok);
    SURoot root=su_hold(clone);if(!ok||!root){su_destroy(clone);su_drop(&root);return 0;}return root;
}
static void fc_warm(void){
    float now=su_number(su.time_unscaled,0);if(now<fc_replenish_at)return;
    if(!fc_template_prepare()){fc_replenish_at=now+1;fc_event("courier-preparation-failed");return;}
    /* Fill one spare per eligible frame, never reconstruct its meshes or PNGs. */
    for(unsigned i=0;i<2;i++)if(!fc_spares[i]){U64 begin=fc_clock();fc_spares[i]=fc_clone_presentation();fc_cost("courier-spare-prepared",begin);if(!fc_spares[i])fc_event("courier-spare-failed");return;}
    if(!fc_warm_reported){fc_warm_reported=1;fc_event("courier-prepared");}
}
static SURoot fc_take_presentation(void){
    for(unsigned i=0;i<2;i++)if(fc_spares[i]){
        SURoot result=fc_spares[i];fc_spares[i]=0;fc_replenish_at=su_number(su.time_unscaled,0)+1;return result;
    }
    return fc_clone_presentation();
}
static int fc_presentation_bind(void){
#define FC_BIND(F,NS,T,N,S) F=su_method(NS,T,N,S);if(!F){fc_event("missing-" T "-" N);return 0;}
    FC_BIND(fc_component_object,"UnityEngine","Component","get_gameObject","");FC_BIND(fc_component_transform,"UnityEngine","Component","get_transform","");
    FC_BIND(fc_get_components,"UnityEngine","GameObject","GetComponentsInternal","System.Type;System.Boolean;System.Boolean;System.Boolean;System.Boolean;System.Object");
    FC_BIND(fc_active,"UnityEngine","GameObject","get_activeSelf","");FC_BIND(fc_set_active,"UnityEngine","GameObject","SetActive","System.Boolean");
    FC_BIND(fc_clone,"UnityEngine","Object","Instantiate","UnityEngine.Object");FC_BIND(fc_find,"UnityEngine","Transform","Find","System.String");
    FC_BIND(fc_component_get,"UnityEngine","Component","GetComponent","System.Type");FC_BIND(fc_behaviour_get,"UnityEngine","Behaviour","get_enabled","");FC_BIND(fc_behaviour_set,"UnityEngine","Behaviour","set_enabled","System.Boolean");
    FC_BIND(fc_on,"MoonCleaners","MoonVehicleLights","get_On","");FC_BIND(fc_time_delta,"UnityEngine","Time","get_deltaTime","");FC_BIND(fc_restoring,"MoonCleaners","MoonWorld","get_IsRestoringRendering","");
    FC_BIND(fc_light_kind,"UnityEngine","Light","set_type","UnityEngine.LightType");FC_BIND(fc_light_range,"UnityEngine","Light","set_range","System.Single");
    FC_BIND(fc_light_angle,"UnityEngine","Light","set_spotAngle","System.Single");FC_BIND(fc_light_intensity,"UnityEngine","Light","set_intensity","System.Single");FC_BIND(fc_light_colour,"UnityEngine","Light","set_color","UnityEngine.Color");
#undef FC_BIND
    fc_world_field=get_field(find_class("MoonCleaners","MoonWalker"),"world");fc_transform_type=su_type("UnityEngine","Transform");fc_light_type=su_type("UnityEngine","Light");
    PTR kernel=LoadLibraryA("kernel32.dll");fc_counter=(int(*)(U64 *))GetProcAddress(kernel,"QueryPerformanceCounter");int (*frequency)(U64 *)=(int(*)(U64 *))GetProcAddress(kernel,"QueryPerformanceFrequency");if(!frequency||!frequency(&fc_frequency))fc_counter=0;
    return fc_world_field&&fc_transform_type&&fc_light_type;
}
