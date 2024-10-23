// Ben Wesch, 2024
// based on https://www.mstarlabs.com/dsp/goertzel/goertzel.html

#include "m_pd.h"
#include <math.h>

static t_class *goertzel_tilde_class;

typedef struct _goertzel_tilde {
    t_object  x_obj;
    t_sample f_dummy;     // Dummy float for signal inlet
    t_float f_frequency;  // Target frequency
    t_float f_realW;      // Precomputed real W
    t_float f_imagW;      // Precomputed imaginary W
    t_float f_sr;        // Stored sample rate
    t_int f_blocksize;   // Stored block size
    t_outlet *x_real_out; // Real part outlet
    t_outlet *x_imag_out; // Imaginary part outlet
} t_goertzel_tilde;

static void goertzel_tilde_update_coefficients(t_goertzel_tilde *x)
{
    float k = (int)(0.5 + ((float)x->f_blocksize * x->f_frequency / x->f_sr));
    x->f_realW = 2.0 * cos(2.0 * M_PI * k / x->f_blocksize);
    x->f_imagW = sin(2.0 * M_PI * k / x->f_blocksize);
}

static t_int *goertzel_tilde_perform(t_int *w)
{
    t_goertzel_tilde *x = (t_goertzel_tilde *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    
    float d1 = 0.0f, d2 = 0.0f, y;
    
    // Process the block using pre-calculated coefficients
    for(int n = 0; n < x->f_blocksize; n++)
    {
        y = in[n] + x->f_realW * d1 - d2;
        d2 = d1;
        d1 = y;
    }
    
    // Calculate real and imaginary parts
    float resultr = 0.5f * x->f_realW * d1 - d2;
    float resulti = x->f_imagW * d1;
    
    // Output real and imaginary parts
    outlet_float(x->x_imag_out, resulti);
    outlet_float(x->x_real_out, resultr);
    
    return (w+3);
}

static void goertzel_tilde_dsp(t_goertzel_tilde *x, t_signal **sp)
{
    // Store sample rate and block size
    x->f_sr = sys_getsr();
    x->f_blocksize = sp[0]->s_n;
    
    goertzel_tilde_update_coefficients(x);
    
    dsp_add(goertzel_tilde_perform, 2, x, sp[0]->s_vec);
}

static void goertzel_tilde_frequency(t_goertzel_tilde *x, t_floatarg f)
{
    if (f >= 0 && f < x->f_sr / 2) {
        x->f_frequency = f;
        goertzel_tilde_update_coefficients(x);
    } else {
        pd_error(x, "goertzel~: frequency must be positive and below Nyquist");
        x->f_frequency = 0;
        goertzel_tilde_update_coefficients(x);
    }
}

static void *goertzel_tilde_new(t_floatarg f)
{
    t_goertzel_tilde *x = (t_goertzel_tilde *)pd_new(goertzel_tilde_class);
    
    // Initialize sample rate and block size
    x->f_sr = sys_getsr();
    x->f_blocksize = sys_getblksize();
    
    // Set initial frequency and calculate coefficients
    goertzel_tilde_frequency(x, f);
    
    x->x_real_out = outlet_new(&x->x_obj, &s_float);
    x->x_imag_out = outlet_new(&x->x_obj, &s_float);
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