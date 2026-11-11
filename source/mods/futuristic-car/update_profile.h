/* Disposable F11 windows only. Observe the camera sampler without owning its
 * controls; write buffered callback costs only after that window has stopped. */
typedef struct {
    U64 total,test,trailer,state,presentation,warm;
    unsigned frame,phase,pacing,instances;
} FCUpdateSample;
static FCUpdateSample fc_update_samples[4096];
static unsigned fc_update_count,fc_update_overflow,fc_update_sampling,fc_update_window;
static void fc_update_profile_sync(void){
    if(!fc_test)return;
    if(fct_profile){
        if(!fc_update_sampling){fc_update_count=fc_update_overflow=0;fc_update_sampling=1;fc_update_window++;}
        return;
    }
    if(!fc_update_sampling)return;fc_update_sampling=0;
    if(!fc_log||!fc_frequency)return;
    char b[768];DWORD n=0;
    for(unsigned i=0;i<fc_update_count;i++){
        FCUpdateSample *s=fc_update_samples+i;
        char *p=cat(b,"{\"event\":\"courier-update-cost\",\"window\":");p=num(p,fc_update_window);
        p=cat(p,",\"unity_frame\":");p=num(p,s->frame);p=cat(p,",\"phase\":");p=num(p,s->phase);
        p=cat(p,",\"pacing\":");p=num(p,s->pacing);p=cat(p,",\"instances\":");p=num(p,s->instances);
#define FC_TIME(NAME,FIELD) p=cat(p,",\"" NAME "_us\":");p=num(p,s->FIELD*1000000/fc_frequency)
        FC_TIME("total",total);FC_TIME("test",test);FC_TIME("trailer",trailer);
        FC_TIME("state",state);FC_TIME("presentation",presentation);FC_TIME("warm",warm);
#undef FC_TIME
        p=cat(p,"}\n");WriteFile(fc_log,b,(DWORD)(p-b),&n,0);
    }
    char *p=cat(b,"{\"event\":\"courier-update-profile-complete\",\"window\":");p=num(p,fc_update_window);
    p=cat(p,",\"samples\":");p=num(p,fc_update_count);p=cat(p,",\"overflow\":");p=num(p,fc_update_overflow);
    p=cat(p,"}\n");WriteFile(fc_log,b,(DWORD)(p-b),&n,0);
}
static void fc_update_profile_sample(U64 begin,U64 tested,U64 controlled,U64 admitted,U64 presented,U64 ended,unsigned instances){
    if(!begin||!fc_test||!fc_update_sampling||!fct_profile)return;
    if(fc_update_count>=4096){fc_update_overflow=1;return;}
    int ok=1;unsigned frame=(unsigned)integer(fct.frame,0,&ok);if(!ok){fc_update_overflow=1;return;}
    /* Total excludes this bookkeeping/frame-ID read and the later dump. These
     * are Courier Update costs, not framework FixedUpdate or renderer clocks. */
    fc_update_samples[fc_update_count++]=(FCUpdateSample){
        ended-begin,tested-begin,controlled-tested,admitted-controlled,
        presented-admitted,ended-presented,frame,
        fct.warming?(fct.warm_frame<360?1u:2u):0u,fct.hardware_sync,instances};
}
