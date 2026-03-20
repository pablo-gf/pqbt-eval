#include <string.h>
#include <stdint.h>
#include "hal.h"
#include "randombytes.h"

// Configure micro-ecc for P-192 and P-256 on ARM Cortex-M4
#define uECC_PLATFORM uECC_arm_thumb2
#define uECC_ARM_USE_UMAAL 1
#define uECC_OPTIMIZATION_LEVEL 2
#define uECC_SQUARE_FUNC 0
#define uECC_SUPPORTS_secp192r1 1
#define uECC_SUPPORTS_secp256r1 1
#define uECC_SUPPORTS_secp224r1 0
#define uECC_SUPPORTS_secp256k1 0
#define uECC_SUPPORT_COMPRESSED_POINT 0

// Include micro-ecc implementationsett
#include "./micro-ecc/uECC.c"

// Wrapper for randombytes to match micro-ecc's RNG callback signature since randombytes returns 0 on success, micro-ecc expects 1 on success
static int rng_callback(uint8_t *dest, unsigned size) {
    return (randombytes(dest, size) == 0) ? 1 : 0; // Ternary operator: condition ? value_if_true : value_if_false
}

void benchmark_curve(const char *curve_name, uECC_Curve curve, int private_key_size, int public_key_size, int shared_secret_size) {
    uint8_t private_key1[private_key_size];
    uint8_t public_key1[public_key_size];
    uint8_t private_key2[private_key_size];
    uint8_t public_key2[public_key_size];
    uint8_t shared_secret[shared_secret_size];
    uint32_t t0, t1;

    // Reduced to 10 iterations for faster testing
    #define NUM_ITERATIONS 100
    unsigned long long keygen_cycles[NUM_ITERATIONS];
    unsigned long long sharedsecret_cycles[NUM_ITERATIONS];
    unsigned long long total_keygen = 0, total_sharedsecret = 0;
    unsigned long long min_keygen = ~0ULL, max_keygen = 0;
    unsigned long long min_sharedsecret = ~0ULL, max_sharedsecret = 0;

    hal_send_str(curve_name);
    hal_send_str("=========================");

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        // Benchmark: Key generation
        t0 = (uint32_t)hal_get_time();
        int result1 = uECC_make_key(public_key1, private_key1, curve);
        t1 = (uint32_t)hal_get_time();

        (void)result1;  // Suppress unused warning

        uint64_t cycles;
        if (t1 >= t0) {
            cycles = t1 - t0;
        } else {
            cycles = (0xFFFFFFFFu - t0 + 1) + t1;
        }
        keygen_cycles[i] = cycles;
        total_keygen += keygen_cycles[i];
        if (keygen_cycles[i] < min_keygen) min_keygen = keygen_cycles[i];
        if (keygen_cycles[i] > max_keygen) max_keygen = keygen_cycles[i];

        // Generate second keypair for shared secret test
        uECC_make_key(public_key2, private_key2, curve);

        // Benchmark: Shared secret computation
        t0 = (uint32_t)hal_get_time();
        uECC_shared_secret(public_key2, private_key1, shared_secret, curve);
        t1 = (uint32_t)hal_get_time();
        if (t1 >= t0) {
            cycles = t1 - t0;
        } else {
            cycles = (0xFFFFFFFFu - t0 + 1) + t1;
        }
        sharedsecret_cycles[i] = cycles;
        total_sharedsecret += sharedsecret_cycles[i];
        if (sharedsecret_cycles[i] < min_sharedsecret) min_sharedsecret = sharedsecret_cycles[i];
        if (sharedsecret_cycles[i] > max_sharedsecret) max_sharedsecret = sharedsecret_cycles[i];
    }

    // Send all individual measurements
    hal_send_str("keygen_raw:");
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        hal_send_number(keygen_cycles[i]);
    }

    hal_send_str("sharedsecret_raw:");
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        hal_send_number(sharedsecret_cycles[i]);
    }

    hal_send_str("#");
}

int main(void) {
    // Setup hardware (clock, UART, etc.)
    hal_setup();

    // Set up RNG for micro-ecc (required for key generation!)
    uECC_set_rng(rng_callback);

    hal_send_str("================================");
    hal_send_str("ECDH Benchmark");
    hal_send_str("================================");

    // Run benchmarks for P-192 and P-256
    benchmark_curve("ECDH P-192", uECC_secp192r1(), 24, 48, 24);
    benchmark_curve("ECDH P-256", uECC_secp256r1(), 32, 64, 32);

    hal_send_str("================================");
    hal_send_str("DONE");
    hal_send_str("================================");

    // Signal completion
    hal_send_str("#");

    // Infinite loop
    while (1);

    return 0;
}