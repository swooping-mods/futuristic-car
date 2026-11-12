static void fc_trailer_update(PTR walker){
    if(!fc_test)return;
    if(fct.returning||(fct.session.token&&!fct.captured)){fct_stop();return;}
    if(fc_key(284)){fct_stop();return;}
    if(fct.session.token&&fct.captured){
        if(fc_key(285)){
            fct_profile=0;fct_dump();
            if(fct_control_down()){
                /* A new pacing mode needs a fresh settled warm-up; retain the
                 * camera lease and its original restoration snapshot. */
                fct.warming=0;if(!fct_pacing(!fct.hardware_sync)){fct_stop();return;}
            }else{fct.cached=!fct.cached;fct_event(fct.cached?"mode-cached":"mode-original-lookups");}
        }
        if(fc_key(286)){fct_profile=0;fct_dump();fct.warm_frame=0;fct.warm_shot=fct.shot;fct.warming=1;fct.shot=1;fct.start=su_number(su.time_unscaled,0);su_float(fct.set_fov,su_get(fct.camera),fct_shots[0].fov);fct_profile=1;fct_event("warmup-start");}
        if(fc_key(292)){if(fct_profile){fct_profile=0;fct_dump();}else{fct_frame_count=0;fct_previous=0;fct_profile=1;fct_event("profile-start");}}
    }
    if(!fc_key(283))return;
    if(!fct.session.token){
        FCInstance *v=0;for(unsigned i=0;i<32;i++)if(fc_instances[i].vehicle&&!fc_instances[i].remote){v=fc_instances+i;break;}
        if(!v||!moon_camera_begin(&fct.session,fct.api,"courier-trailer",fc_field(walker,"world"),walker,fc_field(walker,"viewCamera"),MOON_CAMERA_ALL,fct_capture,walker)){
            if(fct.session.token)fct_stop();fc_event("trailer-camera-unavailable");return;
        }
        fct.vehicle=su_hold(su_get(v->vehicle));
        int ok=1;fct.vehicle_tr=su_hold(call(fct.component_transform,su_get(v->vehicle),0,&ok));
        if(!fct.vehicle_tr||!ok){fct_stop();return;}
        if(!fct_pacing(0)){fct_stop();return;}
    }
    if(!fct.session.token||!fct.captured||fct.returning)return;
    fct.shot=fct.shot%6+1;fct.start=su_number(su.time_unscaled,0);fct_hide_hud();
    su_float(fct.set_fov,su_get(fct.camera),fct_shots[fct.shot-1].fov);
    char b[64],*p=cat(b,"trailer-shot-");num(p,fct.shot);fc_event(b);
}
