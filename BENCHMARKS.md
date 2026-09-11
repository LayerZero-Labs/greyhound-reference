# Benchmark reports

This document separates two experiments:

1. a current comparison of the tight inner-commitment bounds against base
   commit `687a6f8`;
2. the historical dense-sign versus sparse-ternary JL comparison whose
   sparse-ternary endpoint is `687a6f8`.

## Tight inner-commitment bound follow-up

Measured on 2026-09-10 on an Apple M4 Max with 64 GiB RAM, macOS 26.6.2,
Apple Clang 21.0.0, and the portable backend. Runs used eight worker threads
and `LABRADOR_SIS_SECURITY=l2-quantum128-adps16`.

The comparison uses the public repository states directly:

- **Before:** commit `687a6f8`, which uses `6*T*B*s*beta_prime` for the
  Greyhound and LaBRADOR inner commitments.
- **After:** implementation commit `4f419a9`, which uses the tight Greyhound
  bound and the full LaBRADOR Theorem 5.1 maximum described below.

For an `f`-part radix-`2^b` decomposition, define
`B = 2^((f-1)*b)`. Let `beta` be the source relation's public L2 bound,
`beta_prime` the target relation's L2 bound, and
`s = SLACK = sqrt(128/29)`. The current implementation uses:

```text
Greyhound root:
    8*T*(B+1)*s*beta_prime

Recursive LaBRADOR fold:
    max(8*T*(B+1)*s*beta_prime,
        2*(B+1)*s*beta_prime + 4*T*s*beta)

Terminal LaBRADOR fold with a directly checked target witness:
    max(8*T*(B+1)*beta_prime,
        2*(B+1)*beta_prime + 4*T*s*beta)
```

The Greyhound root uses the tight inequality proved in the body of Lemma 2.11
of the [Greyhound paper](https://eprint.iacr.org/2024/1293). The recursive and
terminal formulas implement Theorem 5.1 and Remark 5.2 of the
[LaBRADOR paper](https://eprint.iacr.org/2022/1341). Recursive slack applies to
the `beta_prime` terms; it is unnecessary when the terminal witness is sent to
the verifier and checked directly.

### Proof size and security

Each row reports component-wise medians over five successful matched seeds.
Component medians need not sum to the median total. Sizes are exact contextual
proof bytes. The minimum is taken over every SIS estimate reported during the
five successful after-runs.

| Degree | Total bytes, before/after | Fold bytes, before/after | Tail bytes, before/after | Aggregate JL bytes, before/after | Minimum quantum bits, before/after |
|---:|---:|---:|---:|---:|---:|
| `2^20` | 55,574 / 56,345 (`+1.39%`) | 39,312 / 39,317 | 16,265 / 16,946 | 2,946 / 2,953 | 130.910 / 128.260 |
| `2^21` | 56,677 / 59,328 (`+4.68%`) | 39,596 / 43,066 | 17,051 / 16,257 | 2,972 / 3,349 | 128.525 / 128.260 |
| `2^22` | 59,284 / 60,101 (`+1.38%`) | 43,096 / 43,107 | 16,178 / 16,996 | 3,383 / 3,390 | 129.055 / 128.525 |

### Schedule changes

| Degree | Top shape, before/after | Top rank `kappa/kappa1`, before/after | Pack members, before/after | Current proof-byte range |
|---:|---:|---:|---:|---:|
| `2^20` | `425x39 / 434x38` | `21/7 / 22/7` | `6 / 6–7` | 56,263–58,687 |
| `2^21` | `614x54` | `22/8` | `6–7 / 7` | 59,258–59,356 |
| `2^22` | `868x76 / 887x74` | `22/8 / 23/8` | `7` | 60,016–60,111 |

The first branch of the LaBRADOR maximum dominated every honest fold in these
runs. The source-dependent second branch therefore did not increase the
reported honest schedules, but remains necessary for verifier soundness when a
proof supplies a small target bound relative to the public source bound.

One accepted `2^21` fold used grind nonce 1. Every other accepted root and fold
in the reported samples used nonce 0.

### Sampling record

The matched table samples use seeds `tight-bound-<degree>-<index>` with indices
`1,3,4,5,6` for `2^20` and `1,2,3,4,5` for `2^21` and `2^22`. Every selected
sample passed final verification at both commits.

Two extra current-branch samples completed proving but failed final
verification and are excluded:

- `tight-bound-20-2` failed at both commits. Base returned 125 for the
  aggregated dot-product constraint; the current implementation returned 124
  for an amortized inner-commitment opening.
- `tight-bound-22-7` passed at the base commit but returned 125 for the
  aggregated dot-product constraint at the current commit.

These observations are retained as benchmark outcomes. This report does not
diagnose their cause or treat them as successful samples.

### Reproduce the tight-bound comparison

Build the current portable test binary:

```sh
make BACKEND=portable test_greyhound
```

For example, run sample 1 at degree `2^22` with:

```sh
GREYHOUND_BENCH_SEED=tight-bound-22-1 \
LABRADOR_SIS_SECURITY=l2-quantum128-adps16 \
LATTICE_DOGS_THREADS=8 \
GREYHOUND_BENCH_PACK_ONLY=1 \
./test_greyhound 65536
```

The argument is the number of 64-coefficient polynomials, so 65,536 inputs
represent `65536 * 64 = 2^22` scalar coefficients. The benchmark seed affects
only the test harness; production APIs continue to obtain their initial seed
from `randombytes`.

## Historical sparse-ternary JL comparison at `687a6f8`

Measured on 2026-09-01 on an exe.dev VM with two AMD EPYC 9554P vCPUs,
8 GiB RAM, Ubuntu 24.04, GCC 13.3, and native AVX-512. Builds used
`-O3 -flto=auto -march=native -mtune=native`, two worker threads, and
`LABRADOR_SIS_SECURITY=l2-quantum128-adps16`.

The comparison isolates the JL change:

- **Before:** commit `0c72ba9`, using one dense sign matrix, projected-energy
  multiplier 256, and the historical `SLACK = 2`.
- **After:** commit `687a6f8`, using two independent sign planes realizing
  `A = (S1 + S2) / 2`, projected-energy multiplier 128, certified lower-tail
  multiplier 29, and `SLACK = sqrt(128/29)`.

Both benchmark trees include the same deterministic test-seed hook and the
same one-line unaligned wire-decoder repair. Neither changes the protocol being
compared. Paired runs use identical initial witnesses. Degrees `2^22` and
`2^24` use three paired seeds; `2^26` uses four paired seeds because the first
baseline seed exposed an inherited verification failure, recorded below.

### Standalone JL scaling

Here `n_v` counts scalar coefficients; one `poly` contains 64 coefficients.
Each row is the median of three calls on the same witness and packed matrices.
Derivation times only AES-CTR matrix expansion, projection times only `Jw`, and
collapse times the verifier's matrix transpose/challenge collapse plus ring
conversion.

| `n_v` | Polys | Matrix bytes before/after | Derive before/after | Project before/after | Collapse before/after | Peak RSS |
|---:|---:|---:|---:|---:|---:|---:|
| `2^22` | 65,536 | 128 / 256 MiB | 6.11 / 11.82 ms (`1.93x`) | 28.76 / 35.30 ms (`1.23x`) | 95.55 / 56.31 ms (`0.59x`) | 314 MiB |
| `2^24` | 262,144 | 512 MiB / 1 GiB | 24.45 / 48.83 ms (`2.00x`) | 114.63 / 133.30 ms (`1.16x`) | 330.05 / 209.86 ms (`0.64x`) | 1.22 GiB |
| `2^26` | 1,048,576 | 2 / 4 GiB | 99.31 / 196.49 ms (`1.98x`) | 458.32 / 483.95 ms (`1.06x`) | 1.325 / 0.961 s (`0.73x`) | 4.88 GiB |

Two independent bit planes still cost approximately `2x` to derive. Projection
runs the independent planes concurrently above 16,384 polynomials; its overhead
therefore falls from `1.23x` to `1.06x` as thread-launch cost is amortized. The
collapse kernel accumulates both planes into one integer buffer, halves exactly,
and performs one ring conversion. Independent 16-polynomial output blocks run
across the two configured workers, making its wall time lower than the old
single-thread dense kernel. This is an actual implementation comparison, not a
claim that the ternary arithmetic requires less total CPU work than dense.
A one-worker control measures ternary collapse at `1.11x`, `1.12x`, and
`1.15x` dense time for `2^22`, `2^24`, and `2^26`; the two-worker speedup is
therefore parallel wall-time recovery, while the remaining arithmetic overhead
is only 11–15%.

Peak RSS in this synthetic test includes the witness, both matrices, and the
full collapsed ring vector simultaneously. The protocol's stage-local memory
profile is measured separately below.

### Whole Greyhound Pack timing

Values are medians over the paired successful seeds. `Commit` is shown because
it is a useful control: JL is not used there, so it should remain essentially
unchanged. `Prove` and `verify` include the complete Pack paths, not only JL.

| `n_v` | Commit before/after | Prove before/after | Verify before/after |
|---:|---:|---:|---:|
| `2^22` | 0.141 / 0.134 s (`0.95x`) | 0.285 / 0.279 s (`0.98x`) | 0.172 / 0.156 s (`0.91x`) |
| `2^24` | 0.546 / 0.539 s (`0.99x`) | 0.574 / 0.626 s (`1.09x`) | 0.308 / 0.283 s (`0.92x`) |
| `2^26` | 2.976 / 2.958 s (`0.99x`) | 1.719 / 1.659 s (`0.96x`) | 0.644 / 0.581 s (`0.90x`) |

The only remaining prover regression is `9%` at `2^24`, where the corrected
parameters raise the top outer rank from 8 to 9. At `2^26`, the sparse-ternary
path is still `4%` faster even though the corrected schedule changes the top
shape and sometimes selects an eighth Pack member. Across the four paired
after-runs, proving times were 1.729, 1.650, 1.627, and 1.667 seconds.

One paired-seed run under `/usr/bin/time -v` gives the following whole-process
peak RSS. Unlike the standalone test, Greyhound never holds matrices for the
original `n_v`-dimensional vector at every stage simultaneously.

| `n_v` | Before | After | Increase |
|---:|---:|---:|---:|
| `2^22` | 306.2 MiB | 336.8 MiB | 10.0% |
| `2^24` | 1,043.9 MiB | 1,107.4 MiB | 6.1% |
| `2^26` | 4,623.1 MiB | 4,769.9 MiB | 3.2% |

### Proof size and security parameters

All sizes are exact contextual proof bytes. The table reports medians over the
same paired successful seeds used for timing.

| `n_v` | Total before/after | Fold before/after | Tail before/after | Aggregate JL bytes before/after | Minimum quantum bits before/after |
|---:|---:|---:|---:|---:|---:|
| `2^22` | 59,379 / 59,297 (`-0.14%`) | 43,216 / 43,105 | 16,176 / 16,192 | 3,518 / 3,388 | 130.380 / 129.055 |
| `2^24` | 59,006 / 59,060 (`+0.09%`) | 43,266 / 43,409 | 15,734 / 15,656 | 3,569 / 3,430 | 129.320 / 128.260 |
| `2^26` | 62,382.5 / 63,510.5 (`+1.81%`) | 45,112 / 46,732 | 17,276 / 16,809 | 3,623.5 / 3,677.5 | 129.055 / 132.235 |

For a fixed fold schedule, sparse ternary lowers the Rice-coded JL payload by
about 130 bytes: it has half the coordinate variance. At `2^26`, the aggregate
JL median is slightly larger only because half the after-runs add another fold;
the seven-member after-runs use a median 3,493 JL bytes.

The certified slack changes actual SIS schedules:

| `n_v` | Before top shape/rank | After top shape/rank | Accepted fold ranks before | Accepted fold ranks after |
|---:|---|---|---|---|
| `2^22` | `868x76`, `22/8` | unchanged | `18/6 → 15/5 → 14/5 → 12/4 → 12/4 → 12/4 → 11/0 tail` | unchanged |
| `2^24` | `1774x148`, `23/8` | `1774x148`, `23/9` | `18/6 → 15/5 → 14/5 → 12/4 → 12/4 → 12/4 → 11/0 tail` | same sequence; one seed raises one `12/4` to `12/5` |
| `2^26` | `3547x296`, `23/9` | `3620x290`, `24/9` | `19/7 → 15/6 → 14/5 → 13/5 → 12/4 → 12/4 → 11/0 tail` | `19/7 → 16/6 → 15/5 → 13/5 → 12/4 → 12/4 → [12/4] → 11/0 tail` |

The bracketed `2^26` fold is selected by the greedy size optimizer in two of
the four paired after-runs. Consequently, after-proof totals are bimodal:
62,434–62,512 bytes with seven members and 64,509–64,658 bytes with eight.
The baseline range is 62,317–62,423 bytes.

Every accepted SIS instance remains above the configured 128-bit quantum
ADPS16 floor. The existing JL retry nonce remains only a deterministic candidate
index. No nonce cap or extra security-bit adjustment is introduced.

### Baseline failure retained in the record

The first deterministic `2^26` baseline seed completed proving but failed the
final verifier with `Aggregated dot-product constraint doesn't hold` (return
code 125). The other four baseline seeds and all five sparse-ternary seeds
passed. The failed baseline sample is excluded from timing and size medians but
is not silently converted into a successful observation. This failure is in
the inherited dense-sign baseline and is not evidence for or against the JL
tail bound by itself.

### Reproduce the historical JL comparison

Build the native AVX-512 tests:

```sh
make BACKEND=avx512 test_jlproj test_jlproj_scale test_greyhound
```

Run the standalone workloads; the argument counts 64-coefficient
polynomials:

```sh
./test_jlproj_scale 65536
./test_jlproj_scale 262144
./test_jlproj_scale 1048576
```

Run a deterministic whole-path sample at `n_v = 2^24`:

```sh
GREYHOUND_BENCH_SEED=jl-port-24-1 \
LABRADOR_SIS_SECURITY=l2-quantum128-adps16 \
LATTICE_DOGS_THREADS=2 \
GREYHOUND_BENCH_PACK_ONLY=1 \
./test_greyhound 262144
```

The benchmark seed affects only the test harness. Production APIs continue to
obtain their initial seed from `randombytes`. This report intentionally stops
at `2^26`; `2^27` and `2^28` were not rerun for this comparison.
