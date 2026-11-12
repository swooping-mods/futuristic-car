/* Private director state, shared only with its production regression fixture. */
static struct {
    MoonCameraSession session;
    const MoonCameraApiV1 *api;
    SURoot walker,camera,world,vehicle,camera_tr,vehicle_tr,canvases[128];
    unsigned char enabled[128];
    unsigned count,shot,warm_frame,warm_shot,warming,cached,hardware_sync;
    SPVec3 position;
    SUQuat rotation;
    float fov,start;
    int target_fps,vsync,captured,returning;
    PTR get_fov,set_fov,canvas_enabled,canvas_set,component_transform,world_field,camera_field;
    PTR frame,get_target,set_target,get_vsync,set_vsync;
} fct;
