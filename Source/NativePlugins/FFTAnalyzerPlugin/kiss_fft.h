#ifndef KISS_FFT_H
#define KISS_FFT_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

// --- FIX: DEFINIR MALLOC Y FREE MANUALMENTE PARA EL LINKER ---
#ifndef KISS_FFT_MALLOC
#define KISS_FFT_MALLOC malloc
#endif

#ifndef KISS_FFT_FREE
#define KISS_FFT_FREE free
#endif
// -------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

    /*
     ATTENTION!
     If you would like a :
     -- a utility that will handle the caching of fft objects
     -- real-only (no imaginary time component) FFT
     -- a multi-dimensional FFT
     -- a command-line utility
     check out kiss_fft_utils.h
     */

#ifdef FIXED_POINT
#include <sys/types.h>
#if (FIXED_POINT == 32)
# define kiss_fft_scalar int32_t
#else
# define kiss_fft_scalar int16_t
#endif
#else
# ifndef kiss_fft_scalar
     /* default is float */
#   define kiss_fft_scalar float
# endif
#endif

    typedef struct {
        kiss_fft_scalar r;
        kiss_fft_scalar i;
    } kiss_fft_cpx;

    typedef struct kiss_fft_state* kiss_fft_cfg;

    /*
     * kiss_fft_alloc
     *
     * Initialize a FFT (or IFFT) algorithm's cfg/state buffer.
     *
     * nfft: the dimension of the FFT
     * inverse_fft: 1 for inverse FFT, 0 for forward FFT
     * mem: if null, kiss_fft_alloc will allocate length bytes for the cfg
     * lenmem: if null, kiss_fft_alloc will not check the size of the mem buffer
     */
    kiss_fft_cfg kiss_fft_alloc(int nfft, int inverse_fft, void* mem, size_t* lenmem);

    /*
     * kiss_fft(cfg,in_out_buf)
     *
     * Perform an FFT on a complex input buffer.
     * for a forward FFT,
     * fin should be  f[0] , f[1] , ... ,f[nfft-1]
     * fout will be   F[0] , F[1] , ... ,F[nfft-1]
     * Note that each element is complex and can be accessed like
        f[k].r and f[k].i
     * */
    void kiss_fft(kiss_fft_cfg cfg, const kiss_fft_cpx* fin, kiss_fft_cpx* fout);

    /*
     A helper function to call kiss_fft() with free-ed memory
     */
    void kiss_fft_cleanup(void);

    /*
     * Returns the smallest integer k, such that k>=n and k has only "fast" factors (2,3,5)
     */
    int kiss_fft_next_fast_size(int n);

#ifdef __cplusplus
}
#endif

#endif