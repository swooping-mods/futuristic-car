/* Qualification shortcuts exist only in an explicitly disposable harness. */

static float fc_next_sample;
static SURoot fc_test_owned,fc_test_garage;
static void fc_test_switch(PTR walker,PTR world){
    int ok=1;U64 begin=fc_clock();PTR message=0,result=0;
    if(fc_test_owned){
        void *a[]={su_get(fc_test_owned),su_get(fc_test_garage),&message};
        result=call(su_method("MoonCleaners","MoonWorld","RecallGarageVehicle","MoonCleaners.MoonOwnedVehicle;MoonCleaners.MoonGarage;System.String&"),world,a,&ok);
        int success=ok&&result&&*(unsigned char *)unbox(result);
        su_drop(&fc_test_owned);su_drop(&fc_test_garage);
        fc_event(success?"test-switched-courier":"test-switch-failed");fc_cost("courier-native-switch",begin);return;
    }
    PTR vehicle=0;for(unsigned i=0;i<32;i++)if(!fc_instances[i].remote&&su_alive(su_get(fc_instances[i].vehicle))){vehicle=su_get(fc_instances[i].vehicle);break;}
    if(!vehicle||boolean(su_method("MoonCleaners","MoonPrototypeVehicle","get_IsDriven",""),vehicle,&ok)||!ok)goto fail;
    PTR storage=call(su_method("MoonCleaners","MoonPrototypeVehicle","get_StorageId",""),vehicle,0,&ok);void *at[]={storage};
    fc_test_owned=su_hold(storage?call(su_method("MoonCleaners","MoonWorld","OwnedVehicleAt","System.String"),world,at,&ok):0);
    if(!ok||!fc_test_owned)goto fail;
    /* Keep the exact existing ownership record. Native recall/deployment checks
     * remain authoritative; SpawnVehicle itself only accepts vanilla IDs 0..19. */
    int inactive=0,sort=0;void *find[]={su_type("MoonCleaners","MoonGarage"),&inactive,&sort};
    SURoot garages=su_hold(call(su_method("UnityEngine","Object","FindObjectsByType","System.Type;UnityEngine.FindObjectsInactive;UnityEngine.FindObjectsSortMode"),0,find,&ok));
    U64 count=garages?su.array_length(su_get(garages)):0;PTR nearest=0;float best=1e12f;SPVec3 position=su_vector(su.tr_get_position,fc_transform(walker));
    if(ok&&count<=128)for(U64 i=0;i<count;i++){PTR g=*(PTR *)su.array_address(su_get(garages),8,i);SPVec3 delta=su_sub(su_vector(su.tr_get_position,fc_transform(g)),position);float distance=su_dot(delta,delta);if(distance<best){best=distance;nearest=g;}}
    fc_test_garage=su_hold(nearest);su_drop(&garages);if(!fc_test_garage)goto fail;
    void *recall[]={storage,&message};result=call(su_method("MoonCleaners","MoonWorld","RecallToGarage","System.String;System.String&"),world,recall,&ok);
    if(!ok||!result||!*(unsigned char *)unbox(result))goto fail;
    void *type[]={su_type("MoonCleaners","MoonPrototypeEquipment")};PTR equipment=call(fc_component_get,walker,type,&ok);
    if(!equipment){
        void *find_equipment[]={su_type("MoonCleaners","MoonPrototypeEquipment"),&inactive,&sort};
        SURoot all=su_hold(call(su_method("UnityEngine","Object","FindObjectsByType","System.Type;UnityEngine.FindObjectsInactive;UnityEngine.FindObjectsSortMode"),0,find_equipment,&ok));
        U64 n=all?su.array_length(su_get(all)):0;
        if(ok&&n<=64)for(U64 i=0;i<n;i++){PTR e=*(PTR *)su.array_address(su_get(all),8,i);if(call(su_method("MoonCleaners","MoonPrototypeEquipment","get_Player",""),e,0,&ok)==walker){equipment=e;break;}}
        su_drop(&all);
    }
    if(!equipment||!ok)goto fail;
    int index=0;void *spawn[]={&index};result=call(su_method("MoonCleaners","MoonPrototypeEquipment","SpawnVehicle","System.Int32"),equipment,spawn,&ok);
    if(!ok||!result||!*(unsigned char *)unbox(result))goto fail;
    fc_event("test-switched-vanilla");fc_cost("courier-native-switch",begin);return;
fail:su_drop(&fc_test_owned);su_drop(&fc_test_garage);fc_event("test-switch-failed");
}

static int fc_key(int key){
    static short (*state)(int);static unsigned char previous[12];if(key<282||key>293)return 0;
    if(!state)state=(short(*)(int))GetProcAddress(LoadLibraryA("user32.dll"),"GetAsyncKeyState");if(!state)return 0;
    int slot=key-282,down=(state(0x70+slot)&0x8000)!=0,edge=down&&!previous[slot];previous[slot]=(unsigned char)down;return edge;
}
static char *fc_number(char *p,float f){if(f<0){*p++='-';f=-f;}U64 n=(U64)(f*1000+.5f);p=num(p,n/1000);*p++='.';*p++=(char)('0'+n/100%10);*p++=(char)('0'+n/10%10);*p++=(char)('0'+n%10);*p=0;return p;}
static void fc_probe(FCInstance *v){
    PTR vehicle=su_get(v->vehicle);int ok=1;SPVec3 p=su_vector(su.tr_get_position,fc_transform(vehicle));int driven=boolean(su_method("MoonCleaners","MoonPrototypeVehicle","get_IsDriven",""),vehicle,&ok);char b[2048],*s=cat(b,"{\"event\":\"courier-state\",\"driven\":");s=num(s,(U64)driven);s=cat(s,",\"position\":[");s=fc_number(s,p.x);*s++=',';s=fc_number(s,p.y);*s++=',';s=fc_number(s,p.z);s=cat(s,"],\"seconds\":");s=fc_number(s,su_number(su.time_unscaled,0));s=cat(s,",\"instance\":");s=num(s,(U64)(v-fc_instances));
    MoonHoverState state={0};if(fc_framework->hover_state(vehicle,&state,sizeof(state))){
        s=cat(s,",\"speed\":");s=fc_number(s,rv_length(state.measured));s=cat(s,",\"acceleration\":[");s=fc_number(s,state.flight.acceleration.x);*s++=',';s=fc_number(s,state.flight.acceleration.y);*s++=',';s=fc_number(s,state.flight.acceleration.z);s=cat(s,"]");
    }
    s=cat(s,",\"engines_up\":[");for(unsigned i=0;i<4;i++){SPVec3 up=su_vector(su.tr_up,su_get(v->joints[i]));if(i)*s++=',';*s++='[';s=fc_number(s,up.x);*s++=',';s=fc_number(s,up.y);*s++=',';s=fc_number(s,up.z);*s++=']';}
    s=cat(s,"],\"lights_on\":");s=num(s,(U64)(v->light_state>0));s=cat(s,",\"donor_rig_active\":");s=num(s,v->native_rig?(U64)boolean(fc_active,su_get(v->native_rig),&ok):0);s=cat(s,"}\n");DWORD n=0;WriteFile(fc_log,b,(DWORD)(s-b),&n,0);
}
static void fc_test_update(PTR walker){
    if(!fc_test)return;static int seen;PTR world=fc_field(walker,"world");int ok=1;if(!world||boolean(fc_restoring,world,&ok)||!boolean(su_method("UnityEngine","Application","get_isFocused",""),0,&ok))return;
    if(!seen){seen=1;fc_event("world-ready");}
    if(fc_key(293))fc_test_switch(walker,world);
    if(fc_key(291)){
        int amount=5000;void *a[]={&amount};PTR money=call(su_method("System","Decimal","op_Implicit","System.Int32"),0,a,&ok);if(money){void *set[]={unbox(money)};call(su_method("MoonCleaners","MoonWorld","SetTeamDollars","System.Decimal"),world,set,&ok);}fc_event(ok?"test-funds-set":"test-funds-failed");
    }
    if(fc_key(287)){
        int inactive=0,sort=0;void *a[]={su_type("MoonCleaners","MoonGarage"),&inactive,&sort};PTR array=call(su_method("UnityEngine","Object","FindObjectsByType","System.Type;UnityEngine.FindObjectsInactive;UnityEngine.FindObjectsSortMode"),0,a,&ok);U64 count=array?su.array_length(array):0;PTR nearest=0;float best=1e12f;SPVec3 p=su_vector(su.tr_get_position,fc_transform(walker));
        for(U64 i=0;i<count&&i<128;i++){PTR g=*(PTR *)su.array_address(array,8,i);SPVec3 delta=su_sub(su_vector(su.tr_get_position,fc_transform(g)),p);float dist=su_dot(delta,delta);if(dist<best){best=dist;nearest=g;}}
        void *type[]={su_type("MoonCleaners","MoonGarageMenu")};PTR menu=call(su_method("UnityEngine","Component","GetComponent","System.Type"),walker,type,&ok);int id=nearest?integer(su_method("MoonCleaners","MoonGarage","get_Id",""),nearest,&ok):-1;void *open[]={&id};PTR result=menu&&nearest?call(su_method("MoonCleaners","MoonGarageMenu","Open","System.Int32"),menu,open,&ok):0;fc_event(ok&&result&&*(unsigned char *)unbox(result)?"test-garage-opened":"test-garage-unavailable");
    }
    int stage=fc_key(288),focus=fc_key(289);FCInstance *v=0;for(unsigned i=0;i<32;i++)if(!fc_instances[i].remote&&su_alive(su_get(fc_instances[i].vehicle))){v=fc_instances+i;break;}
    if(v&&(stage||focus)){
        PTR vt=fc_transform(su_get(v->vehicle));SPVec3 p=su_vector(su.tr_get_position,vt),up=su_vector(su.tr_up,vt);
        if(stage){
            SPVec3 point=su_sub(su_sub(p,su_mul(su_vector(su.tr_forward,vt),5)),su_mul(su_vector(su.tr_right,vt),3));SPVec3 origin=su_add(point,su_mul(up,8)),down=su_mul(up,-1);float distance=20;int mask=~(1<<2),trigger=1;union{U64 align;unsigned char bytes[96];}hit;su_zero(&hit,sizeof(hit));void *ray[]={&origin,&down,hit.bytes,&distance,&mask,&trigger};PTR found=call(su.physics_ray,0,ray,&ok);
            if(found&&*(unsigned char *)unbox(found)){SPVec3 floor;su_copy(&floor,hit.bytes+su.hit_point,12);PTR body=fc_field(walker,"body");su_set_vector(su_method("UnityEngine","Rigidbody","set_position","UnityEngine.Vector3"),body,su_add(floor,su_mul(up,.2f)));su_set_vector(su_method("UnityEngine","Rigidbody","set_linearVelocity","UnityEngine.Vector3"),body,su_v(0,0,0));fc_event("test-side-staged");}
        }
        PTR camera=call(su.camera_main,0,0,&ok);SPVec3 eye=su_vector(su.tr_get_position,fc_transform(camera)),dir=su_normal(su_sub(su_add(p,su_mul(up,1.1f)),eye));void *look[]={&dir};call(su_method("MoonCleaners","MoonWalker","TestLook","UnityEngine.Vector3"),walker,look,&ok);fc_probe(v);fc_event(ok?"test-car-focused":"test-focus-failed");
    }
    float now=su_number(su.time_unscaled,0);if(v&&now>=fc_next_sample){fc_next_sample=now+.5f;fc_probe(v);}
}
