/* Same boxes create the fitted collision hull and its clearance samples.
 * Coordinates include the presentation child's +0.3 m offset. */
#include "../vehicle-framework/sdk/hover.h"
static const MoonHoverBox fc_hover_boxes[]={
    {"Courier cabin",{0,1.35f,0},{1.9f,1.8f,3.6f}},
    /* Bound the entire gimbal/heave envelope plus at least 20mm clearance.
     * These same conservative hulls drive terrain sensing, not animated rays. */
    {"Front left pod",{-1.28f,.64f,1.33f},{.74f,.86f,1.14f}},
    {"Front right pod",{1.28f,.64f,1.33f},{.74f,.86f,1.14f}},
    {"Rear left pod",{-1.31f,.64f,-1.43f},{.70f,.86f,1.12f}},
    {"Rear right pod",{1.31f,.64f,-1.43f},{.70f,.86f,1.12f}}
};
static const MoonHoverProfile fc_hover_profile={
    .abi=MV_HOVER_ABI,.size=sizeof(MoonHoverProfile),
    .tuning={.hover_height=.65f,.cruise_speed=40,.boost_speed=72,.reverse_speed=14,.strafe_speed=18,.vertical_speed=14,
        .acceleration=12,.boost_acceleration=18,.brake_acceleration=24,.jerk=80,
        .turn_rate=1.25f,.turn_falloff=.018f,.turn_response=8,.bank_factor=.18f,
        .tilt_scale=34,.max_tilt=.42f,.terrain_tilt=1.73f,.attitude_response=8,.terrain_response=10,
        .heave_acceleration=21,.heave_response=12,.height_response=3,.probe_max=200},
    .boxes=fc_hover_boxes,.box_count=5,.reference=0,.replace_native_solids=1,.flight_enabled=1
};
