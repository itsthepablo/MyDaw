#include "kiss_fft.h"
#include "_kiss_fft_guts.h"
/* The guts header contains all the multiplication and addition macros that are defined for
 fixed or floating point complex numbers.  It also contains the hard definitions
 for scalar types. */

static void kf_bfly2(
    kiss_fft_cpx* Fout,
    const size_t fstride,
    const kiss_fft_cfg st,
    int m
)
{
    kiss_fft_cpx* Fout2;
    kiss_fft_cpx* tw1 = st->twiddles;
    kiss_fft_cpx t;
    Fout2 = Fout + m;
    do {
        C_MUL(t, *Fout2, *tw1);
        tw1 += fstride;
        C_SUB(*Fout2, *Fout, t);
        C_ADDTO(*Fout, t);
        ++Fout;
        ++Fout2;
    } while (--m);
}

static void kf_bfly4(
    kiss_fft_cpx* Fout,
    const size_t fstride,
    const kiss_fft_cfg st,
    const size_t m
)
{
    kiss_fft_cpx* tw1, * tw2, * tw3;
    kiss_fft_cpx scratch[6];
    size_t k = m;
    const size_t m2 = 2 * m;
    const size_t m3 = 3 * m;


    tw3 = tw2 = tw1 = st->twiddles;

    do {
        C_MUL(scratch[0], Fout[m], *tw1);
        C_MUL(scratch[1], Fout[m2], *tw2);
        C_MUL(scratch[2], Fout[m3], *tw3);

        C_SUB(scratch[5], *Fout, scratch[1]);
        C_ADDTO(*Fout, scratch[1]);
        C_ADD(scratch[3], scratch[0], scratch[2]);
        C_SUB(scratch[4], scratch[0], scratch[2]);
        C_SUB(Fout[m2], *Fout, scratch[3]);
        tw1 += fstride;
        tw2 += fstride * 2;
        tw3 += fstride * 3;
        C_ADDTO(*Fout, scratch[3]);

        if (st->inverse) {
            Fout[m].r = scratch[5].r - scratch[4].i;
            Fout[m].i = scratch[5].i + scratch[4].r;
            Fout[m3].r = scratch[5].r + scratch[4].i;
            Fout[m3].i = scratch[5].i - scratch[4].r;
        }
        else {
            Fout[m].r = scratch[5].r + scratch[4].i;
            Fout[m].i = scratch[5].i - scratch[4].r;
            Fout[m3].r = scratch[5].r - scratch[4].i;
            Fout[m3].i = scratch[5].i + scratch[4].r;
        }
        ++Fout;
    } while (--k);
}

static void kf_bfly3(
    kiss_fft_cpx* Fout,
    const size_t fstride,
    const kiss_fft_cfg st,
    size_t m
)
{
    size_t k = m;
    const size_t m2 = 2 * m;
    kiss_fft_cpx* tw1, * tw2;
    kiss_fft_cpx scratch[5];
    kiss_fft_cpx epi3;
    epi3 = st->twiddles[fstride * m];

    tw1 = tw2 = st->twiddles;

    do {
        C_MUL(scratch[1], Fout[m], *tw1);
        C_MUL(scratch[2], Fout[m2], *tw2);

        C_ADD(scratch[3], scratch[1], scratch[2]);
        C_SUB(scratch[0], scratch[1], scratch[2]);
        tw1 += fstride;
        tw2 += fstride * 2;

        Fout[m].r = Fout->r - HALF_OF(scratch[3].r);
        Fout[m].i = Fout->i - HALF_OF(scratch[3].i);

        C_MULBYSCALAR(scratch[0], epi3.i);

        C_ADDTO(*Fout, scratch[3]);

        Fout[m2].r = Fout[m].r + scratch[0].i;
        Fout[m2].i = Fout[m].i - scratch[0].r;

        Fout[m].r -= scratch[0].i;
        Fout[m].i += scratch[0].r;

        ++Fout;
    } while (--k);
}

static void kf_bfly5(
    kiss_fft_cpx* Fout,
    const size_t fstride,
    const kiss_fft_cfg st,
    int m
)
{
    kiss_fft_cpx* Fout0, * Fout1, * Fout2, * Fout3, * Fout4;
    int u;
    kiss_fft_cpx scratch[13];
    kiss_fft_cpx* twiddles = st->twiddles;
    kiss_fft_cpx* tw;
    kiss_fft_cpx ya, yb;
    ya = twiddles[fstride * m];
    yb = twiddles[fstride * 2 * m];

    Fout0 = Fout;
    Fout1 = Fout0 + m;
    Fout2 = Fout0 + 2 * m;
    Fout3 = Fout0 + 3 * m;
    Fout4 = Fout0 + 4 * m;

    tw = st->twiddles;
    for (u = 0; u < m; ++u) {
        C_MUL(scratch[0], *Fout1, *tw);
        C_MUL(scratch[1], *Fout2, tw[fstride]);
        C_MUL(scratch[2], *Fout3, tw[fstride * 2]);
        C_MUL(scratch[3], *Fout4, tw[fstride * 3]);
        tw++;

        C_ADD(scratch[7], scratch[0], scratch[3]);
        C_SUB(scratch[10], scratch[0], scratch[3]);
        C_ADD(scratch[8], scratch[1], scratch[2]);
        C_SUB(scratch[9], scratch[1], scratch[2]);

        Fout0->r += scratch[7].r + scratch[8].r;
        Fout0->i += scratch[7].i + scratch[8].i;

        scratch[5].r = scratch[0].r + scratch[2].r + scratch[1].r + scratch[3].r;
        scratch[5].i = scratch[0].i + scratch[2].i + scratch[1].i + scratch[3].i;

        /* CORRECCIN AQU
: Usamos S_MUL para multiplicar escalares en lugar de la macro compleja */
        /* Esto arregla el error "left operand of .i must have struct/union type" y los errores de sintaxis */

        Fout2->r = Fout0->r + S_MUL(scratch[7].r, ya.r) + S_MUL(scratch[8].r, yb.r);
        Fout2->i = Fout0->i + S_MUL(scratch[7].i, ya.r) + S_MUL(scratch[8].i, yb.r);

        Fout3->r = Fout0->r + S_MUL(scratch[10].r, ya.i) + S_MUL(scratch[9].r, yb.i);
        Fout3->i = Fout0->i + S_MUL(scratch[10].i, ya.i) + S_MUL(scratch[9].i, yb.i);

        C_ADD(*Fout1, *Fout2, *Fout3);
        C_SUB(*Fout4, *Fout2, *Fout3);

        Fout0++;
        Fout1++;
        Fout2++;
        Fout3++;
        Fout4++;
    }
}

/* kf_bfly_generic
 *
 * This function has been included for completeness' sake.
 * In practice, you should probably probably limit the FFT lengths to those
 * supported by the butterflies above.
 */
static void kf_bfly_generic(
    kiss_fft_cpx* Fout,
    const size_t fstride,
    const kiss_fft_cfg st,
    int m,
    int p
)
{
    int u, k, q1, q;
    kiss_fft_cpx* twiddles = st->twiddles;
    kiss_fft_cpx t;
    int Norig = st->nfft;

    kiss_fft_cpx* scratch = (kiss_fft_cpx*)KISS_FFT_MALLOC(sizeof(kiss_fft_cpx) * p);

    for (u = 0; u < m; ++u) {
        k = u;
        for (q1 = 0; q1 < p; ++q1) {
            scratch[q1] = Fout[k];
            C_FIXDIV(scratch[q1], p);
            k += m;
        }

        k = u;
        for (q1 = 0; q1 < p; ++q1) {
            int twidx = 0;
            Fout[k] = scratch[0];
            for (q = 1;q < p;++q) {
                twidx += fstride * k;
                if (twidx >= Norig) twidx -= Norig;
                C_MUL(t, scratch[q], twiddles[twidx]);
                C_ADDTO(Fout[k], t);
            }
            k += m;
        }
    }
    KISS_FFT_FREE(scratch);
}

static
void kf_work(
    kiss_fft_cpx* Fout,
    const kiss_fft_cpx* f,
    const size_t fstride,
    int in_stride,
    int* factors,
    const kiss_fft_cfg st
)
{
    kiss_fft_cpx* Fout_beg = Fout;
    const int p = *factors++; /* the radix  */
    const int m = *factors++; /* stage's fft length/p */
    const kiss_fft_cpx* Fout_end = Fout + p * m;

#ifdef _OPENMP
    // use OpenMP extensions at the top-level (not recursive)
    if (fstride == 1 && p <= 5)
    {
        int k;

        // execute the p different work units of length m
#       pragma omp parallel for
        for (k = 0;k < p;++k)
            kf_work(Fout + k * m, f + fstride * in_stride * k, fstride * p, in_stride, factors, st);
        // all threads must be complete before butterfly
#       pragma omp barrier
    }
    else
#endif
    {
        if (m == 1) {
            do {
                *Fout = *f;
                f += fstride * in_stride;
            } while (++Fout != Fout_end);
        }
        else {
            do {
                // recursive call:
                // DFT of size m*p performed by doing
                // p instances of smaller DFTs of size m, 
                // each one takes a decimated version of the input
                kf_work(Fout, f, fstride * p, in_stride, factors, st);
                f += fstride * in_stride;
            } while ((Fout += m) != Fout_end);
        }
    }

    Fout = Fout_beg;

    // recombine the p smaller DFTs 
    switch (p) {
    case 2: kf_bfly2(Fout, fstride, st, m); break;
    case 3: kf_bfly3(Fout, fstride, st, m); break;
    case 4: kf_bfly4(Fout, fstride, st, m); break;
    case 5: kf_bfly5(Fout, fstride, st, m); break;
    default: kf_bfly_generic(Fout, fstride, st, m, p); break;
    }
}

/* facbuf is populated by p1,m1,p2,m2, ...
    where
    p[i] * m[i] = m[i-1]
    m0 = n                 */
static
void kf_factor(int n, int* facbuf)
{
    int p = 4;
    double floor_sqrt;
    floor_sqrt = floor(sqrt((double)n));

    /*factor out powers of 4, powers of 2, and any other
      prime factors up to sqrt(n)*/
    do {
        while (n % p) {
            switch (p) {
            case 4: p = 2; break;
            case 2: p = 3; break;
            default: p += 2; break;
            }
            if (p > floor_sqrt)
                p = n;          /* no more factors, skip to end */
        }
        n /= p;
        *facbuf++ = p;
        *facbuf++ = n;
    } while (n > 1);
}

/*
 *
 * kiss_fft_alloc
 *
 * Initialize a FFT (or IFFT) algorithm's cfg/state buffer.
 *
 * nfft: the dimension of the FFT
 * inverse_fft: 1 for inverse FFT, 0 for forward FFT
 * mem: if null, kiss_fft_alloc will allocate length bytes for the cfg
 * lenmem: if null, kiss_fft_alloc will not check the size of the mem buffer
 *
 */
kiss_fft_cfg kiss_fft_alloc(int nfft, int inverse_fft, void* mem, size_t* lenmem)
{
    kiss_fft_cfg st = NULL;
    size_t memneeded = sizeof(struct kiss_fft_state)
        + sizeof(kiss_fft_cpx) * (nfft - 1); /* twiddle factors*/

    if (lenmem == NULL) {
        st = (kiss_fft_cfg)KISS_FFT_MALLOC(memneeded);
    }
    else {
        if (mem != NULL && *lenmem >= memneeded)
            st = (kiss_fft_cfg)mem;
        *lenmem = memneeded;
    }
    if (st) {
        int i;
        st->nfft = nfft;
        st->inverse = inverse_fft;

        for (i = 0;i < nfft;++i) {
            const double pi = 3.141592653589793238462643383279502884197169399375105820974944;
            double phase = -2 * pi * i / nfft;
            if (st->inverse)
                phase *= -1;
            kf_cexp(st->twiddles + i, phase);
        }

        kf_factor(nfft, st->factors);
    }
    return st;
}


void kiss_fft(kiss_fft_cfg cfg, const kiss_fft_cpx* fin, kiss_fft_cpx* fout)
{
    /* input buffer stride */
    int in_stride = 1;
    if (cfg->inverse) in_stride = 2; // ?
    kf_work(fout, fin, 1, in_stride, cfg->factors, cfg);
}