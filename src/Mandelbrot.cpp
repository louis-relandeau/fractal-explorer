#include "Mandelbrot.hpp"

#include <immintrin.h>  // For AVX intrinsics

std::uint32_t mandelbrotFunction(double cr, double ci) {
    const int maxIterations = 1000;
    double zr = 0.0, zi = 0.0;
    double zr2 = 0.0, zi2 = 0.0;
    
    int n = 0;
    while (zr2 + zi2 <= 4.0 && n < maxIterations) {
        zi = 2.0 * zr * zi + ci;
        zr = zr2 - zi2 + cr;
        zr2 = zr * zr;
        zi2 = zi * zi;
        ++n;
    }
    
    if (n == maxIterations) {
        return 0x000000;
    } else {
        unsigned char color = static_cast<unsigned char>(255 * n / maxIterations);
        return (color << 16) | (color << 8) | color;
    }
}

// AVX version - processes 4 pixels simultaneously
void mandelbrotAVX(const double* cr, const double* ci, std::uint32_t* results) {
    const int maxIterations = 1000;
    
    // Load 4 values at once (256-bit register holds 4 doubles)
    __m256d cr_vec = _mm256_loadu_pd(cr);  // cr[0], cr[1], cr[2], cr[3]
    __m256d ci_vec = _mm256_loadu_pd(ci);  // ci[0], ci[1], ci[2], ci[3]
    
    // Initialize vectors for 4 pixels
    __m256d zr = _mm256_setzero_pd();      // [0.0, 0.0, 0.0, 0.0]
    __m256d zi = _mm256_setzero_pd();
    __m256d zr2 = _mm256_setzero_pd();
    __m256d zi2 = _mm256_setzero_pd();
    
    __m256d four = _mm256_set1_pd(4.0);    // [4.0, 4.0, 4.0, 4.0]
    __m256d two = _mm256_set1_pd(2.0);     // [2.0, 2.0, 2.0, 2.0]
    
    // Iteration counters for each pixel
    int n[4] = {0, 0, 0, 0};
    int active[4] = {1, 1, 1, 1};  // Which pixels are still iterating
    
    for (int iter = 0; iter < maxIterations; iter++) {
        // Check which pixels have escaped (r^2 > 4.0)
        __m256d r2 = _mm256_add_pd(zr2, zi2);
        __m256d mask = _mm256_cmp_pd(r2, four, _CMP_LE_OQ);
        
        // If all pixels have escaped, stop
        if (_mm256_movemask_pd(mask) == 0) break;
        
        // Mandelbrot iteration for all 4 pixels at once
        // zi_new = 2 * zr * zi + ci
        __m256d zi_new = _mm256_mul_pd(two, _mm256_mul_pd(zr, zi));
        zi_new = _mm256_add_pd(zi_new, ci_vec);
        
        // zr_new = zr^2 - zi^2 + cr
        __m256d zr_new = _mm256_sub_pd(zr2, zi2);
        zr_new = _mm256_add_pd(zr_new, cr_vec);
        
        // Update z values
        zr = zr_new;
        zi = zi_new;
        zr2 = _mm256_mul_pd(zr, zr);
        zi2 = _mm256_mul_pd(zi, zi);
        
        // Update iteration counts for active pixels
        double mask_array[4];
        _mm256_storeu_pd(mask_array, mask);
        for (int i = 0; i < 4; i++) {
            if (mask_array[i] != 0.0 && active[i]) {
                n[i]++;
            } else {
                active[i] = 0;
            }
        }
    }
    
    // Convert iteration counts to colors
    for (int i = 0; i < 4; i++) {
        if (n[i] == maxIterations) {
            results[i] = 0x000000;
        } else {
            unsigned char color = static_cast<unsigned char>(255 * n[i] / maxIterations);
            results[i] = (color << 16) | (color << 8) | color;
        }
    }
}