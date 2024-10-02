#include "m_pd.h"
#include <math.h>

static t_class *goertzel_tilde_class;

typedef struct _goertzel_tilde {
    t_object  x_obj;
    t_sample f;
    t_outlet *x_out;
    t_float coeff;
    t_float q0;
    t_float q1;
    t_float q2;
    t_float sine;
    t_float cosine;
    t_float frequency;
    t_float sr;
} t_goertzel_tilde;

static t_int *goertzel_tilde_perform(t_int *w)
{
    t_goertzel_tilde *x = (t_goertzel_tilde *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    
    t_float q0 = x->q0;
    t_float q1 = x->q1;
    t_float q2 = x->q2;
    t_float coeff = x->coeff;
    
    while (n--) {
        q0 = coeff * q1 - q2 + *in++;
        q2 = q1;
        q1 = q0;
        
        *out++ = q0;
    }
    
    x->q0 = q0;
    x->q1 = q1;
    x->q2 = q2;
    
    return (w+5);
}

static void goertzel_tilde_dsp(t_goertzel_tilde *x, t_signal **sp)
{
    dsp_add(goertzel_tilde_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

static void goertzel_tilde_frequency(t_goertzel_tilde *x, t_floatarg f)
{
    x->frequency = f;
    t_float omega = 2.0 * M_PI * x->frequency / x->sr;
    x->coeff = 2.0 * cosf(omega);
    x->sine = sinf(omega);
    x->cosine = cosf(omega);
}

static void *goertzel_tilde_new(t_floatarg f)
{
    t_goertzel_tilde *x = (t_goertzel_tilde *)pd_new(goertzel_tilde_class);
    
    x->sr = sys_getsr();
    x->frequency = f;
    
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("frequency"));
    x->x_out = outlet_new(&x->x_obj, &s_signal);
    
    goertzel_tilde_frequency(x, f);
    
    return (void *)x;
}

void goertzel_tilde_setup(void) {
    goertzel_tilde_class = class_new(gensym("goertzel~"),
        (t_newmethod)goertzel_tilde_new,
        0, sizeof(t_goertzel_tilde),
        CLASS_DEFAULT,
        A_DEFFLOAT, 0);
    
    class_addmethod(goertzel_tilde_class,
        (t_method)goertzel_tilde_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(goertzel_tilde_class,
        (t_method)goertzel_tilde_frequency, gensym("frequency"), A_FLOAT, 0);
    CLASS_MAINSIGNALIN(goertzel_tilde_class, t_goertzel_tilde, f);
}