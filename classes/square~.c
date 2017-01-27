// Porres 2016

#include "m_pd.h"
#include "math.h"


static t_class *square_class;

typedef struct _square
{
    t_object x_obj;
    double  x_phase;
    double  x_last_phase_offset;
    t_float  x_freq;
    t_inlet  *x_inlet_phase;
    t_inlet  *x_inlet_sync;
    t_outlet *x_outlet;
    t_float x_sr;
} t_square;

static t_int *square_perform(t_int *w)
{
    t_square *x = (t_square *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // freq
    t_float *in2 = (t_float *)(w[4]); // phase
    t_float *in3 = (t_float *)(w[5]); // sync
    t_float *out = (t_float *)(w[6]);
    double phase = x->x_phase;
    double last_phase_offset = x->x_last_phase_offset;
    double sr = x->x_sr;
    double output;
    while (nblock--)
    {
        double hz = *in1++;
        double phase_offset = *in2++;
        double trig = *in3++;
        double phase_step = hz / sr; // phase_step
        phase_step = phase_step > 0.5 ? 0.5 : phase_step < -0.5 ? -0.5 : phase_step; // clipped to nyq
        double phase_dev = phase_offset - last_phase_offset;
        if (phase_dev >= 1 || phase_dev <= -1)
            phase_dev = fmod(phase_dev, 1); // fmod(phase_dev)
        if (hz >= 0)
            {
                if (trig > 0 && trig <= 1) phase = trig;
                else
                    {
                        phase = phase + phase_dev;
                        if (phase <= 0) phase = phase + 1.; // wrap deviated phase
                    }
                    if (phase >= 1)
                        {
                        output = 1; // 1st sample is always 1
                        phase = phase - 1; // wrapped phase
                        }
                    else if (phase + phase_step >= 1) output = -1; // last sample is always -1
                    else output = phase <= 0.5 ? 1 : -1;
            }
        else
            {
                if (trig > 0 && trig < 1) phase = trig;
                else if (trig == 1) phase = 0;
                else
                {
                    phase = phase + phase_dev;
                    if (phase >= 1) phase = phase - 1.; // wrap deviated phase
                }
                    if (phase <= 0)
                        {
                        output = -1; // 1st sample is always -1
                        phase = phase + 1; // wrapped phase
                        }
                    else if (phase + phase_step <= 0) output = 1; // last sample is always 1
                    else output = phase <= 0.5 ? 1 : -1;
            }
        *out++ = output;
        phase = phase + phase_step; // next phase
        last_phase_offset = phase_offset; // last phase offset
    }
    x->x_phase = phase;
    x->x_last_phase_offset = last_phase_offset;
    return (w + 7);
}

static void square_dsp(t_square *x, t_signal **sp)
{
    dsp_add(square_perform, 6, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *square_free(t_square *x)
{
    inlet_free(x->x_inlet_phase);
    inlet_free(x->x_inlet_sync);
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *square_new(t_floatarg f1, t_floatarg f2)
{
    t_square *x = (t_square *)pd_new(square_class);
    t_float init_freq = f1;
    t_float init_phase = f2;
    init_phase < 0 ? 0 : init_phase >= 1 ? 0 : init_phase; // clipping phase input
    if (init_phase == 0 && init_freq > 0)
        x->x_phase = 1.;
    x->x_last_phase_offset = 0;
    x->x_freq = init_freq;
    x->x_sr = sys_getsr(); // sample rate
    x->x_inlet_phase = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_phase, init_phase);
    x->x_inlet_sync = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_sync, 0);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void square_tilde_setup(void)
{
    square_class = class_new(gensym("square~"),
        (t_newmethod)square_new, (t_method)square_free,
        sizeof(t_square), CLASS_DEFAULT, A_DEFFLOAT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(square_class, t_square, x_freq);
    class_addmethod(square_class, (t_method)square_dsp, gensym("dsp"), A_CANT, 0);
}
