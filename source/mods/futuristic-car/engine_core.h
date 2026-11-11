#ifndef FUTURISTIC_COURIER_ENGINE_CORE_H
#define FUTURISTIC_COURIER_ENGINE_CORE_H
#include "../vehicle-framework/sdk/hover_core.h"
typedef struct {float pitch,roll,heave,pitch_rate,roll_rate,heave_rate;} FCEngine;
/* Articulation follows measured/control acceleration and yaw, rather than a
 * free-running wobble. The conservative rigid pod hull remains authoritative. */
static inline void fc_engine_step(FCEngine *s,RVVec local_force,float yaw,int engine,float dt){
    if(!s||!rv_valid(local_force)||!rv_finite(yaw)||!rv_finite(dt)||dt<=0||dt>.1f)return;
    float side=(engine&1)?1.f:-1.f,front=engine<2?1.f:-1.f;
    float pitch=rv_clamp(local_force.z*.012f-side*yaw*.04f,-.19f,.19f);
    float roll=rv_clamp(local_force.x*.012f+front*yaw*.025f,-.16f,.16f);
    float lift=rv_clamp(local_force.y*.0015f+front*pitch*.035f+side*roll*.035f,-.018f,.018f);
    if(rv_abs(pitch)<.0005f)pitch=0;if(rv_abs(roll)<.0005f)roll=0;if(rv_abs(lift)<.0001f)lift=0;
    rv_spring(&s->pitch,&s->pitch_rate,pitch,10,dt);
    rv_spring(&s->roll,&s->roll_rate,roll,10,dt);
    rv_spring(&s->heave,&s->heave_rate,lift,9,dt);
    if(!pitch&&rv_abs(s->pitch)<.00001f&&rv_abs(s->pitch_rate)<.0001f)s->pitch=s->pitch_rate=0;
    if(!roll&&rv_abs(s->roll)<.00001f&&rv_abs(s->roll_rate)<.0001f)s->roll=s->roll_rate=0;
    if(!lift&&rv_abs(s->heave)<.00001f&&rv_abs(s->heave_rate)<.0001f)s->heave=s->heave_rate=0;
}
#endif
