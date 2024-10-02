#include "m_pd.h"
#include <math.h>

static t_class *goertzel_tilde_class;

typedef struct _goertzel_tilde {
    t_object  x_obj;
    t_sample f_dummy;     // Dummy float for signal inlet
    t_float f_frequency;  // Target frequency
    t_int   f_n;          // Block size
    t_outlet *x_out;      // Float outlet
} t_goertzel_tilde;

static t_int *goertzel_tilde_perform(t_int *w)
{
    t_goertzel_tilde *x = (t_goertzel_tilde *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    
    float floatnumSamples = (float)n;
    float k = (int)(0.5 + ((floatnumSamples * x->f_frequency) / sys_getsr()));
    float omega = (2.0 * M_PI * k) / floatnumSamples;
    float sine = sinf(omega);
    float cosine = cosf(omega);
    float coeff = 2.0 * cosine;
    float q0 = 0, q1 = 0, q2 = 0;
    
    // Process the block
    for(int i = 0; i < n; i++)
    {
        q0 = coeff * q1 - q2 + in[i];
        q2 = q1;
        q1 = q0;
    }
    
    // Calculate magnitude
    float scalingFactor = n / 2.0;
    float real = (q1 * cosine - q2) / scalingFactor;
    float imag = (q1 * sine) / scalingFactor;
    float magnitude = sqrtf(real*real + imag*imag);
    
    // Output the magnitude
    outlet_float(x->x_out, magnitude);
    
    return (w+4);
}

static void goertzel_tilde_dsp(t_goertzel_tilde *x, t_signal **sp)
{
    x->f_n = sp[0]->s_n;
    dsp_add(goertzel_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void goertzel_tilde_frequency(t_goertzel_tilde *x, t_floatarg f)
{
    if (f > 0 && f < sys_getsr() / 2) {
        x->f_frequency = f;
    } else {
        pd_error(x, "goertzel~: frequency must be positive and below Nyquist");
    }
}

static void *goertzel_tilde_new(t_floatarg f)
{
    t_goertzel_tilde *x = (t_goertzel_tilde *)pd_new(goertzel_tilde_class);
    
    x->f_frequency = (f > 0 && f < sys_getsr() / 2) ? f : 440;
    x->f_n = 64;  // Will be updated in _dsp method
    
    x->x_out = outlet_new(&x->x_obj, &s_float);
    return (void *)x;
}

void goertzel_tilde_setup(void)
{
    goertzel_tilde_class = class_new(gensym("goertzel~"),
        (t_newmethod)goertzel_tilde_new,
        0,
        sizeof(t_goertzel_tilde),
        CLASS_DEFAULT,
        A_DEFFLOAT, 0);
    
    class_addmethod(goertzel_tilde_class, (t_method)goertzel_tilde_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(goertzel_tilde_class, (t_method)goertzel_tilde_frequency, gensym("frequency"), A_FLOAT, 0);
    CLASS_MAINSIGNALIN(goertzel_tilde_class, t_goertzel_tilde, f_dummy);
}
