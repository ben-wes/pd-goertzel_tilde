#include "m_pd.h"
#include <math.h>

static t_class *goertzel_tilde_class;

typedef struct _goertzel_tilde {
    t_object  x_obj;
    t_sample f_dummy;     // Dummy float for signal inlet
    t_float f_frequency;  // Target frequency
    t_int   f_blocksize;  // Current block size
    t_int   f_windowsize; // Number of samples in the analysis window
    t_sample *f_buffer;   // Ring buffer to store samples
    t_int   f_bufferindex; // Current index in buffer
    t_int   f_samplecount; // Total number of samples processed
    t_outlet *x_out;      // Float outlet
} t_goertzel_tilde;

static void goertzel_tilde_analyse(t_goertzel_tilde *x)
{
    float floatnumSamples = (float)x->f_windowsize;
    float k = (int)(0.5 + ((floatnumSamples * x->f_frequency) / sys_getsr()));
    float omega = (2.0 * M_PI * k) / floatnumSamples;
    float sine = sinf(omega);
    float cosine = cosf(omega);
    float coeff = 2.0 * cosine;
    float q0 = 0, q1 = 0, q2 = 0;
    
    // Process the samples in the ring buffer
    for(int i = 0; i < x->f_windowsize; i++)
    {
        int index = (x->f_bufferindex - x->f_windowsize + i + x->f_windowsize) % x->f_windowsize;
        q0 = coeff * q1 - q2 + x->f_buffer[index];
        q2 = q1;
        q1 = q0;
    }
    
    // Calculate magnitude
    float scalingFactor = x->f_windowsize / 2.0;
    float real = (q1 * cosine - q2) / scalingFactor;
    float imag = (q1 * sine) / scalingFactor;
    float magnitude = sqrtf(real*real + imag*imag);
    
    // Output the magnitude
    outlet_float(x->x_out, magnitude);
}

static t_int *goertzel_tilde_perform(t_int *w)
{
    t_goertzel_tilde *x = (t_goertzel_tilde *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    
    // Process incoming samples
    for (int i = 0; i < n; i++) {
        // Add new sample to the buffer
        x->f_buffer[x->f_bufferindex] = in[i];
        x->f_bufferindex = (x->f_bufferindex + 1) % x->f_windowsize;
        x->f_samplecount++;
        
        // If we've accumulated enough samples, perform analysis
        if (x->f_samplecount >= x->f_windowsize) {
            goertzel_tilde_analyse(x);
        }
    }
    
    return (w+4);
}

static void goertzel_tilde_dsp(t_goertzel_tilde *x, t_signal **sp)
{
    x->f_blocksize = sp[0]->s_n;
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

static void goertzel_tilde_windowsize(t_goertzel_tilde *x, t_floatarg f)
{
    int size = (int)f;
    if (size > 0) {
        x->f_windowsize = size;
        x->f_buffer = (t_sample *)resizebytes(x->f_buffer, 
                                              x->f_windowsize * sizeof(t_sample),
                                              size * sizeof(t_sample));
        x->f_bufferindex = 0;
        x->f_samplecount = 0;
    } else {
        pd_error(x, "goertzel~: window size must be positive");
    }
}

static void *goertzel_tilde_new(t_floatarg f, t_floatarg size)
{
    t_goertzel_tilde *x = (t_goertzel_tilde *)pd_new(goertzel_tilde_class);
    
    x->f_frequency = (f > 0 && f < sys_getsr() / 2) ? f : 440;
    x->f_windowsize = (size > 0) ? size : 1024;  // Default window size
    x->f_buffer = (t_sample *)getbytes(x->f_windowsize * sizeof(t_sample));
    x->f_bufferindex = 0;
    x->f_samplecount = 0;
    
    x->x_out = outlet_new(&x->x_obj, &s_float);
    return (void *)x;
}

static void goertzel_tilde_free(t_goertzel_tilde *x)
{
    if (x->f_buffer) freebytes(x->f_buffer, x->f_windowsize * sizeof(t_sample));
}

void goertzel_tilde_setup(void)
{
    goertzel_tilde_class = class_new(gensym("goertzel~"),
        (t_newmethod)goertzel_tilde_new,
        (t_method)goertzel_tilde_free,
        sizeof(t_goertzel_tilde),
        CLASS_DEFAULT,
        A_DEFFLOAT, A_DEFFLOAT, 0);
    
    class_addmethod(goertzel_tilde_class, (t_method)goertzel_tilde_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(goertzel_tilde_class, (t_method)goertzel_tilde_frequency, gensym("frequency"), A_FLOAT, 0);
    class_addmethod(goertzel_tilde_class, (t_method)goertzel_tilde_windowsize, gensym("windowsize"), A_FLOAT, 0);
    CLASS_MAINSIGNALIN(goertzel_tilde_class, t_goertzel_tilde, f_dummy);
}
