# Greyhound Reference

Greyhound Reference is an independent research fork of the Greyhound
polynomial commitment scheme and its Labrador folding backend. It is derived
from the
[lattice-dogs/labrador](https://github.com/lattice-dogs/labrador) implementation
at commit `8b6626b`. It also retains that repository's Chihuahua and Dachshund
front ends.

The primary goal is to provide a transparent and reproducible comparison base
for Greyhound and [Akita](https://github.com/LayerZero-Labs/akita). In
particular, the fork can select parameters under the same 128-bit quantum
ADPS16 core-SVP cost target, print every concrete Euclidean SIS instance, and
measure the exact context-dependent proof bytes that a verifier receives.

The secondary goal is portability. The upstream implementation is optimized
for AVX-512; this fork adds a generic C/SIMDe backend and portable C NTT so the
protocol can be run and inspected on machines without AVX-512. The portable
path favors clarity, coverage, and acceptable reference performance over
architecture-specific optimization.

This is not an official upstream Greyhound or Labrador release.

## Comparison methodology

The Akita comparison mode follows four rules:

1. **Security target:** every Greyhound/Labrador Module-SIS instance is checked
   against a 128-bit quantum floor using the ADPS16 core-SVP cost model. The
   estimator uses Greyhound/Labrador's native Euclidean (L2) collision bound;
   it does not substitute an infinity-norm estimate.
2. **Exact proof bytes:** reported sizes come from canonical serialization, not
   entropy formulas or in-memory object sizes.
3. **Like-for-like context:** public commitments and an agreed parameter
   schedule are treated as verifier context rather than charged to one proof
   but not the other. Self-describing archival framing is reported separately.
4. **Visible parameters:** each fold prints its decomposition, ranks, norm
   bounds, JL data, SIS dimensions, block size, and estimated quantum cost so
   the comparison can be audited rather than inferred from a headline number.

The schemes do not have identical proof components, so the repository exposes
the accounting boundary explicitly instead of claiming a one-to-one mapping.
The security mode is a concrete parameter-estimation policy, not an end-to-end
security proof or implementation audit.

## Current tight-bound reference results

The current implementation was measured locally on an Apple M4 Max with the
portable backend, eight worker threads, and the `l2-quantum128-adps16` policy.
The completed rows compare five successful, matched deterministic runs at base
commit `687a6f8` with the tight-bound implementation at `4f419a9`. Sizes are
exact contextual proof bytes. Minimum-security values cover only the SIS
instances included in accepted proof members.

| Degree | Median proof bytes, base -> current | Change | Top shape and rank `kappa/kappa1`, base -> current | Current minimum quantum bits |
|---:|---:|---:|---:|---:|
| `2^20` | 55,574 -> 56,345 | +1.39% | `425x39 21/7 -> 434x38 22/7` | 128.260 |
| `2^21` | 56,677 -> 59,328 | +4.68% | `614x54 22/8` | 128.260 |
| `2^22` | 59,284 -> 60,101 | +1.38% | `868x76 22/8 -> 887x74 23/8` | 129.320 |
| `2^23` | 58,974 -> 60,413 | +2.44% | `1254x105 23/8` | 130.645 |
| `2^24` | 59,543 -> 60,453 | +1.53% | `1774x148 23/9 -> 1736x152 22/8` | 128.525 |
| `2^25` | 60,920 -> no completed proof in 20 minutes | — | `2560x205 24/9 -> 2455x214 22/9` | — |
| `2^26` | 62,419 -> 63,063 | +1.03% | `3620x290 24/9 -> 4160x253 32/9` | 128.260 |

At `2^25`, five fixed-head seeds each remained CPU-bound in the root
folded-response computation for at least 20 minutes without producing an
accepted root proof. Successful base measurements completed in seconds after
replacing one failed seed. No current proof size or security value is reported
for that degree. The source-dependent branch of the LaBRADOR maximum did not
dominate any completed honest fold. One accepted `2^21` fold used grind nonce
1; all other accepted current folds used
nonce 0. See [BENCHMARKS.md](BENCHMARKS.md) for component medians, proof-byte
ranges, the full sampling and failure record, reproduction commands, and the
historical sparse-ternary JL comparison at `687a6f8`.

## What this fork adds

- A scalar-capable generic C/SIMDe backend for non-AVX-512 machines, including
  Apple silicon.
- Portable C NTT kernels selected automatically outside x86-64.
- Parallel extension-ring products with a configurable worker count.
- Explicit, per-fold parameter and Module-SIS audit reports.
- A selectable Euclidean SIS policy targeting 128-bit quantum security under
  the ADPS16 core-SVP cost model.
- Protocol-derived collision bounds shared by parameter selection, fold
  grinding, verifier checks, and audit output.
- Tight context-dependent proof serialization and separate self-describing
  archival serialization.
- Round-trip, canonical-encoding, truncation, and estimator regression tests.

`BACKEND=auto` is the default: it preserves the optimized upstream backend on
x86-64 and selects the portable backend elsewhere. On an x86-64 machine
without AVX-512, select the generic path explicitly with `BACKEND=portable`.
`BACKEND=avx512` explicitly requests the upstream assembly path.

## Build

Clone with the pinned SIMDe submodule, then build the tests:

```sh
git clone --recurse-submodules https://github.com/quangvdao/greyhound-reference.git
cd greyhound-reference
make
```

To force the generic backend on any supported architecture:

```sh
make BACKEND=portable
```

For an existing checkout, initialize dependencies with:

```sh
git submodule update --init
```

The build requires a C2x compiler, POSIX threads, GMP, and OpenSSL. The code has
been tested on Apple silicon using Apple Clang. `make libdogs.so` builds the
shared library.

## Run Greyhound

`test_greyhound` accepts the number of 64-coefficient input polynomials. Thus,
the following runs a degree-2^20 instance:

```sh
./test_greyhound 16384
```

With no argument, it runs the original degree-2^25 instance. For benchmark
runs, `GREYHOUND_BENCH_PACK_ONLY=1` skips the preliminary standalone
polynomial-commitment test. Large extension products use all online CPUs by
default; set `LATTICE_DOGS_THREADS` to a positive integer to cap the worker
count.

For example:

```sh
LATTICE_DOGS_THREADS=8 \
LABRADOR_SIS_SECURITY=l2-quantum128-adps16 \
GREYHOUND_BENCH_PACK_ONLY=1 \
./test_greyhound 16384
```

Each fold reports its algebraic dimensions, digit decompositions, commitment
ranks, norm bounds, JL projection data, exact proof payload, and SIS estimate.

## JL projection

Norm proofs use a 256-row sparse-ternary matrix. Each entry is sampled exactly
as

```text
A = (S1 + S2) / 2,
```

where `S1` and `S2` are independent packed sign matrices. Thus an entry is
`-1`, `0`, or `1` with probabilities `1/4`, `1/2`, and `1/4`. The prover uses
two calls to the optimized sign-projection kernel. The verifier collapses both
packed planes into one accumulator and performs only one ring conversion.

The accepted projected squared norm is at most `128 * beta^2`. The certified
lower-tail multiplier is 29, so SIS parameter selection uses the exact slack
factor `sqrt(128/29)`, approximately `2.1009`. The transcript domain is
`GREYHOUND-JL-TERNARY-V1`; self-describing proof envelopes use wire version 4.
The existing JL nonce remains a deterministic retry index for finding an
accepted projection. This change introduces no separate nonce cap or decoder
policy.

## SIS security policy

The default `legacy-heuristic` policy retains the upstream closed-form rank
predicate, but applies it to the current collision bounds. It does not reproduce
the old schedule after a bound changes. Set
`LABRADOR_SIS_SECURITY=l2-quantum128-adps16` to require every concrete
Greyhound/Labrador Module-SIS instance to meet a 128-bit quantum floor under
the local Euclidean SIS estimator and the ADPS16 quantum core-SVP cost
`log2(operations) = 0.265 * beta`.

The estimator receives the collision bound required by the corresponding
extraction argument. Write `B = 2^((f-1)*b)`, let `beta` be the source
relation's public L2 bound, let `beta_prime` be the target relation's L2 bound,
and let `s = sqrt(128/29)` be the implementation's JL slack. The bounds are:

```text
Greyhound root:
    8*T*(B+1)*s*beta_prime

Recursive Labrador fold:
    max(8*T*(B+1)*s*beta_prime,
        2*(B+1)*s*beta_prime + 4*T*s*beta)

Terminal Labrador fold with a directly checked target witness:
    max(8*T*(B+1)*beta_prime,
        2*(B+1)*beta_prime + 4*T*s*beta)
```

The Greyhound expression uses the tight `2*kappa_bar*beta_bar` inequality
proved in the body of Greyhound Lemma 2.11. The Labrador expressions implement
Theorem 5.1 and apply Remark 5.2 only when the target norm is recursively
certified. The verifier obtains `beta` from the public input statement and
combines it with the target norm carried by the proof; the prover cannot choose
both sides of the comparison.

The selected inner and outer commitment ranks are increased until all matrix
roles pass, and verification repeats the same checks. Every norm-producing
fold—the Greyhound root, ordinary Labrador levels, and the terminal level—also
grinds a transcript-bound 32-bit nonce until the realized response satisfies
all applicable inner and outer SIS predicates. Ordinary levels keep their
commitments fixed and recompute only the folding challenges and `z`. Nonce zero
preserves the original transcript exactly; retries are domain-separated. The
terminal level keeps `t` fixed and recomputes its dependent sequential `h`,
challenge, and `z` chain. Search is deterministic from nonce zero and capped at
4096 attempts per level. Reports include the scalar SIS dimensions, Euclidean
collision bound, optimized lattice dimension, block size `beta`, and estimated
quantum cost. Unknown nonempty policy names fail closed.

This is a concrete parameter-estimation policy, not a claim that the full
protocol or implementation has received a security audit. Run its regression
vectors with:

```sh
make test_sis_estimator
./test_sis_estimator
```

## Serialization

Two deliberately distinct encodings are available:

- The `*_contextual_*` APIs encode the tight proof payload. The decoder receives
  the public commitment and agreed fold schedule as trusted context, so the
  wire does not repeat magic bytes, versions, lengths, shape tables, schedule
  parameters, or Greyhound's already-public `u1` commitment.
- The unsuffixed `*_serialized_size`, `*_serialize`, and `*_deserialize` APIs
  provide versioned, self-describing archival envelopes. Their framing is not
  counted as proof size.

Both encodings use canonical bit packing. JL coordinates and the terminal
witness use uniquely selected size-minimizing Golomb-Rice parameters. The
Greyhound test checks byte-identical contextual decode/re-encode, rejects
noncanonical and truncated proofs, and verifies the decoded proof.

Run the focused wire-format tests with:

```sh
make test_proof_wire
./test_proof_wire
```

## License and provenance

The upstream implementation is Copyright 2024 IBM Corp. This fork preserves
the Apache License 2.0 and records its provenance in `NOTICE`. See `LICENSE` for
the license text.
