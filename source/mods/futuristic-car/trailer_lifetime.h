static int fct_read(PTR box,void *out,U64 size,int *ok){
    PTR data=*ok&&box?unbox(box):0;if(!data){*ok=0;return 0;}su_copy(out,data,size);return 1;
}
static int fct_finite(float value){return value>=-3.402823466e38F&&value<=3.402823466e38F;}
static int fct_capture(void *context){
    PTR walker=context;int ok=1;fct.captured=0;
    fct.walker=su_hold(walker);fct.world=su_hold(fc_field(walker,"world"));
    PTR camera=fc_field(walker,"viewCamera"),tr=call(fct.component_transform,camera,0,&ok);
    fct.camera_tr=su_hold(tr);
    fct.camera=su_hold(camera);
    if(!su_get(fct.walker)||!su_get(fct.world)||!su_get(fct.camera)||!su_get(fct.camera_tr)||!ok)goto failed;
    fct_read(call(su.tr_get_position,tr,0,&ok),&fct.position,sizeof(fct.position),&ok);
    fct_read(call(su.tr_get_rotation,tr,0,&ok),&fct.rotation,sizeof(fct.rotation),&ok);
    fct_read(call(fct.get_fov,camera,0,&ok),&fct.fov,sizeof(fct.fov),&ok);
    fct_read(call(fct.get_target,0,0,&ok),&fct.target_fps,sizeof(fct.target_fps),&ok);
    fct_read(call(fct.get_vsync,0,0,&ok),&fct.vsync,sizeof(fct.vsync),&ok);
    if(!ok||!fct_finite(fct.position.x)||!fct_finite(fct.position.y)||!fct_finite(fct.position.z)||
       !fct_finite(fct.rotation.x)||!fct_finite(fct.rotation.y)||!fct_finite(fct.rotation.z)||!fct_finite(fct.rotation.w)||
       !(fct.fov>0&&fct.fov<180))goto failed;
    fct.captured=1;return 1;
failed:
    su_drop(&fct.walker);su_drop(&fct.world);su_drop(&fct.camera);su_drop(&fct.camera_tr);return 0;
}
static int fct_restore(void *context){
    (void)context;
    /* A failed read-only capture owes only lease release, never restoration
     * from a partial pose/lens/pacing baseline. */
    if(!fct.captured)return 1;
    int ok=1;PTR camera=su_get(fct.camera);if(!camera)return 0;
    int living=moon_camera_target_alive(su.obj_alive,camera,&ok);if(!ok)return 0;
    if(living){
        PTR tr=su_get(fct.camera_tr);if(!tr)return 0;void *p[]={&fct.position},*q[]={&fct.rotation},*f[]={&fct.fov};
        call(su.tr_position,tr,p,&ok);call(su.tr_rotation,tr,q,&ok);call(fct.set_fov,camera,f,&ok);
    }if(!ok)return 0;
    void *fps[]={&fct.target_fps},*vsync[]={&fct.vsync};call(fct.set_target,0,fps,&ok);call(fct.set_vsync,0,vsync,&ok);
    if(!ok)return 0;
    for(unsigned i=0;i<fct.count;i++){
        PTR canvas=su_get(fct.canvases[i]);if(!canvas)return 0;
        int exists=moon_camera_target_alive(su.obj_alive,canvas,&ok);if(!ok)return 0;
        if(exists){unsigned char enabled=fct.enabled[i];void *a[]={&enabled};call(fct.canvas_set,canvas,a,&ok);if(!ok)return 0;}
    }
    return ok;
}
static void fct_stop(void){
    if(!fct.session.token)return;
    int captured=fct.captured;
    fct.returning=1;if(!moon_camera_finish(&fct.session,fct_restore,0))return;
    fct_profile=0;if(captured)fct_dump();else{fct_frame_count=0;fct_previous=0;}fct.warming=0;
    for(unsigned i=0;i<fct.count;i++)su_drop(fct.canvases+i);fct.count=0;
    su_drop(&fct.walker);su_drop(&fct.world);su_drop(&fct.camera);su_drop(&fct.vehicle);su_drop(&fct.camera_tr);su_drop(&fct.vehicle_tr);fct.shot=0;fct.captured=fct.returning=0;
    fct_event(captured?"camera-restored":"camera-capture-aborted");
    fc_event(captured?"trailer-camera-restored":"trailer-camera-capture-aborted");
}
