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

Completed rows report component-wise medians over five successful matched
seeds. Component medians need not sum to the median total. Sizes are exact
contextual proof bytes. Aggregate JL bytes count only coordinates serialized in
accepted proof members; candidates rejected by the greedy size optimizer are
excluded. The security value is the minimum over the SIS instances in those
accepted proof members.

| Degree | Total bytes, before/after | Fold bytes, before/after | Tail bytes, before/after | Aggregate JL bytes, before/after | Minimum quantum bits, before/after |
|---:|---:|---:|---:|---:|---:|
| `2^20` | 55,574 / 56,345 (`+1.39%`) | 39,312 / 39,317 | 16,265 / 16,946 | 2,566 / 2,571 | 130.910 / 128.260 |
| `2^21` | 56,677 / 59,328 (`+4.68%`) | 39,596 / 43,066 | 17,051 / 16,257 | 2,594 / 2,971 | 128.525 / 128.260 |
| `2^22` | 59,284 / 60,101 (`+1.38%`) | 43,096 / 43,107 | 16,178 / 16,996 | 3,001 / 3,012 | 129.055 / 129.320 |
| `2^23` | 58,974 / 60,413 (`+2.44%`) | 43,125 / 43,642 | 15,897 / 16,779 | 3,030 / 3,035 | 128.260 / 130.645 |
| `2^24` | 59,543 / 60,453 (`+1.53%`) | 43,910 / 43,671 | 15,642 / 16,782 | 3,053 / 3,064 | 128.260 / 128.525 |
| `2^25` | 60,920 / — | 44,466 / — | 16,458 / — | 3,091 / — | 128.525 / — |
| `2^26` | 62,419 / 63,063 (`+1.03%`) | 45,005 / 45,027 | 17,362 / 17,981 | 3,118 / 3,140 | 131.970 / 128.260 |

### Schedule changes

| Degree | Top shape, before/after | Top rank `kappa/kappa1`, before/after | Pack members, before/after | Current proof-byte range |
|---:|---:|---:|---:|---:|
| `2^20` | `425x39 / 434x38` | `21/7 / 22/7` | `6 / 6–7` | 56,263–58,687 |
| `2^21` | `614x54` | `22/8` | `6–7 / 7` | 59,258–59,356 |
| `2^22` | `868x76 / 887x74` | `22/8 / 23/8` | `7` | 60,016–60,111 |
| `2^23` | `1254x105` | `23/8` | `6–7 / 7` | 60,345–60,488 |
| `2^24` | `1774x148 / 1736x152` | `23/9 / 22/8` | `7` | 60,407–60,500 |
| `2^25` | `2560x205 / 2455x214` (attempted) | `24/9 / 22/9` | `7 / —` | — |
| `2^26` | `3620x290 / 4160x253` | `24/9 / 32/9` | `7–8` | 62,938–65,028 |

The first branch of the LaBRADOR maximum dominated every completed honest fold
in these runs. The source-dependent second branch therefore did not increase
the completed schedules, but remains necessary for verifier soundness when a
proof supplies a small target bound relative to the public source bound.

One accepted `2^21` fold used grind nonce 1. Every other accepted root and fold
in the reported samples used nonce 0.

### The `2^25` bounded-run outcome

The fixed implementation selected top shape `2455x214`, decomposition bases
64/64, expansion factors 5/5, ranks `22/9`, and a predicted witness norm of
about 57,578. Five seeds (`tight-bound-25-1` through `tight-bound-25-5`) each
remained CPU-bound in the root folded-response computation for at least 20
minutes without producing an accepted root proof. The processes were then
terminated. A sampled stack placed execution in `polcom_eval`'s
`polxvec_polx_mul_add` call, which computes the response tested by the root
grinding predicate. Because the accepted nonce is printed only after that
computation succeeds, these runs do not distinguish one exceptionally costly
candidate from many retries.

At the base commit, the corresponding top schedule is `2560x205`, ranks
`24/9`. Successful base proofs complete in seconds. The base medians in the
table use seeds 1, 3, 4, 5, and 6 because seed 2 completed proving but failed
final verification. An exploratory fixed-head seed 6 was also terminated
without a root proof after 16 minutes.

This is a benchmark result, not a proof-size estimate. No fixed-head proof
bytes or accepted-instance security value are reported for `2^25`.

### Sampling and failure record

Seeds have the form `tight-bound-<degree>-<index>`. The five successful matched
pairs used for each completed row are:

| Degree | Included indices |
|---:|---|
| `2^20` | 1, 3, 4, 5, 6 |
| `2^21` | 1, 2, 3, 4, 5 |
| `2^22` | 1, 2, 3, 4, 5 |
| `2^23` | 1, 2, 3, 4, 5 |
| `2^24` | 1, 2, 3, 5, 6 |
| `2^26` | 1, 2, 4, 5, 6 |

The following completed proofs failed final verification and are excluded:

- `tight-bound-20-2` failed at both commits. Base returned 125 for the
  aggregated dot-product constraint; the current implementation returned 124
  for an amortized inner-commitment opening.
- `tight-bound-22-7` passed at the base commit but returned 125 for the
  aggregated dot-product constraint at the current commit.
- `tight-bound-24-4` returned 124 for an amortized inner-commitment opening at
  the base commit and passed at the current commit.
- `tight-bound-25-2` returned 124 for an amortized inner-commitment opening at
  the base commit; its current run did not complete within 20 minutes.
- `tight-bound-26-3` passed at the base commit but returned 124 for an
  amortized inner-commitment opening at the current commit.

These observations are retained as benchmark outcomes. This report does not
diagnose their cause or treat them as successful samples.

### Reproduce the tight-bound comparison

Build the current portable test binary:

```sh
make BACKEND=portable test_greyhound
```

For example, run sample 1 at degree `2^26` with:

```sh
GREYHOUND_BENCH_SEED=tight-bound-26-1 \
LABRADOR_SIS_SECURITY=l2-quantum128-adps16 \
LATTICE_DOGS_THREADS=8 \
GREYHOUND_BENCH_PACK_ONLY=1 \
./test_greyhound 1048576
```

The argument is the number of 64-coefficient polynomials, so 1,048,576 inputs
represent `1048576 * 64 = 2^26` scalar coefficients. The benchmark seed affects
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
