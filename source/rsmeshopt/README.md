# rsmeshopt
Triangle strip / triangle fan generation algorithms

## WARNING: Clang or GCC is required to build!

## Rust API
The following Rust API is provided:
```rs
pub fn stripify(algo: u32, indices: &[u32], positions: &[f32], restart: u32) -> Vec<u32>;
pub fn make_fans(indices: &[u32], restart: u32, min_len: u32, max_runs: u32) -> Vec<u32>;
```

## C Bindings
The following C bindings are provided:

```c
uint32_t rii_stripify(uint32_t* dst, uint32_t algo, const uint32_t* indices,
                      uint32_t num_indices, const float* positions,
                      uint32_t num_positions, uint32_t restart);

uint32_t rii_makefans(uint32_t* dst, const uint32_t* indices,
                      uint32_t num_indices, uint32_t restart, uint32_t min_len,
                      uint32_t max_runs);
```
