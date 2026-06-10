# MDCT Audio Compression — Assignment

## Background

The **Modified Discrete Cosine Transform (MDCT)** is a variant of the DCT widely used in audio coding (MP3, AAC, Vorbis, Opus). It takes `2N` samples `(x_0, ..., x_{2N-1})` as input and produces `N` coefficients `(X_0, ..., X_{N-1})` as output:

```
        2N-1
X_k =    Σ   x_n · w_n · cos[ (π/N) · (n + 1/2 + N/2) · (k + 1/2) ]
        n=0
```

for `k ∈ [0, N-1]`, where `w_n` is an edge-attenuation window with the appropriate properties for perfect reconstruction. A commonly used choice is the **sine window**:

```
w_n = sin[ (π / 2N) · (n + 1/2) ]
```

This window satisfies the **Princen–Bradley condition** `w_n² + w_{n+N}² = 1`, which is required to cancel time-domain aliasing during overlap-add.

The **inverse transform** is:

```
              N-1
y_n = (2/N) · w_n · Σ   X_k · cos[ (π/N) · (n + 1/2 + N/2) · (k + 1/2) ]
              k=0
```

for `n ∈ [0, 2N-1]`.

The inverse cannot reconstruct the original `2N` values from a single block of `N` coefficients. However, by feeding the MDCT overlapping input windows (50% overlap) and **adding** the reconstructed outputs (also with 50% overlap), the original signal is recovered exactly — up to small rounding errors.

To allow the first and last input windows to be transformed and reconstructed, the input signal is extended with **`N` zeros before and after**.

## Implementation notes

- The mathematical definition uses real numbers — implement with `double`.
- Input samples are signed 16-bit integers (little-endian, mono).
- Quantized coefficients can be stored as signed 32-bit integers.

## Input

A file `test.raw` containing mono audio samples stored as little-endian 16-bit signed integers.

## Task 1 — Direct sample quantization

1. Measure the entropy of the original signal.
2. Quantize the samples by dividing them by a quantization factor `Q` (use `Q = 2600`).
3. Measure the entropy of the quantized signal.
4. Rebuild the data by de-quantizing and save it as `output_qt.raw`.
5. Compute the per-sample error as the difference from the original signal and save it as `error_qt.raw`.

You can then load `output_qt.raw` and `error_qt.raw` in Audacity:
- Listening to them **together** reconstructs the original signal.
- Listening to them **separately** lets you hear the result of quantization and the quantization error in isolation.

## Task 2 — MDCT-based compression

1. Apply the forward MDCT to the input file with a window of `N = 1024` samples.
2. Quantize the coefficients by dividing them by a quantization factor `Q` (use `Q = 10000`).
3. Measure the entropy of the quantized coefficients.
4. Rebuild the data by de-quantizing the coefficients.
5. Apply the inverse MDCT to the reconstructed coefficients (with 50% overlap-add) and save the resulting signal as `output.raw`.
6. Compute the per-sample error as the difference from the original signal and save it as `error.raw`.

## Output files

| File | Content |
|------|---------|
| `output_qt.raw` | De-quantized signal (direct sample quantization) |
| `error_qt.raw`  | Per-sample error of direct quantization |
| `output.raw`    | Signal reconstructed via inverse MDCT |
| `error.raw`     | Per-sample error of the MDCT pipeline |

All output files use the same format as the input: mono, 16-bit signed PCM, little-endian.

## Build and run

```bash
g++ -O3 -march=native -ffast-math -std=c++20 main.cpp -o mdct
./mdct
```

Place `test.raw` in the same directory as the executable before running.

## Expected output

The program prints to stdout:

- Number of samples in the input file
- Entropy of the original signal (bits/sample)
- Entropy of the quantized signal (bits/sample)
- Entropy of the quantized MDCT coefficients (bits/coefficient)