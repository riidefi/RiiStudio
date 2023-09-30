# rsmeshopt
Triangle strip / triangle fan generation algorithms

## WARNING: Clang or GCC is required to build!
##### Windows build instructions:
```sh
SET CXX=clang
cargo build
```
This will force the compiler to be `clang`, supposing it exists in your path.
##### Linux/Mac build instructions:
```
cargo build
```

## Rust API
The following Rust API is provided:
```rs
pub fn stripify(algo: u32, indices: &[u32], positions: &[f32], restart: u32) -> Vec<u32>;
pub fn make_fans(indices: &[u32], restart: u32, min_len: u32, max_runs: u32) -> Vec<u32>;
```

## C Bindings
The following C bindings are [provided](https://github.com/riidefi/RiiStudio/blob/master/source/rsmeshopt/include/rsmeshopt.h):

```c
uint32_t rii_stripify(uint32_t* dst, uint32_t algo, const uint32_t* indices,
                      uint32_t num_indices, const float* positions,
                      uint32_t num_positions, uint32_t restart);

uint32_t rii_makefans(uint32_t* dst, const uint32_t* indices,
                      uint32_t num_indices, uint32_t restart, uint32_t min_len,
                      uint32_t max_runs);
```

## C# Bindings
The following C# bindings are [provided](https://github.com/riidefi/RiiStudio/tree/master/source/rsmeshopt/c%23):

```cs
public static List<uint> Stripify(StripifyAlgo algo, List<uint> indexData,
                                  List<Vec3> vertexData, uint restart = 0xFFFFFFFF);

public static List<uint> MakeFans(List<uint> indexData, uint restart,
                                  uint minLen, uint maxRuns);
```

## Triangle Stripifying Algorithms

| Enum | Source | Notes |
|------|--------|-------|
| NvTriStrip | amorilia/tristrip (NvTriStrip fork) | Fork of amorilia/tristrip's fork of NVTriStrip (early 2000s NVidia freeware), notably lacking cache support. This is actually fine since our target doesn't actually *have* a post-TnL cache. The best algorithm here > 50% of the time. |
| Draco | google/draco | One of the fastest algorithms here and often ties with NvTriStrip for compression rate. In some cases, it can outperform NvTriStrip. |
| Haroohie | jellees/nns-blender-plugin (Gericom & Jelle) | Should yield equivalent result to N files (at least on DS models). Consequently, it's the slowest and outclassed by other algorithms here (it never wins). This is only half of the library (rewritten in C++), because the GC/Wii does not support quadstrips. Here for historical/decompilation reasons. |
| TriStripper | GPSnoopy/TriStripper | This is the library BrawlBox uses. In maybe 5% of models it wins, but usually does not. |
| MeshOptmzr | zeux/meshoptimizer | When given garbage geometry, it can outperform the other algorithms here. In most cases, however, it is not as aggressive as the other algorithms here so not as suitable for GC/Wii. (it is designed for modern GPUs with large post-TnL caches and tristrips are not nearly as relevant for modern desktop GPU work) |
| DracoDegen | google/draco | Alternative version of the draco algorithm that does not use primitive restart. Mainly for Android phones with quirky GPUs--we have the equivalent of primitive restart on GC/Wii so do not need this. |
| RiiFans | riidefi/RiiStudio | Entirely bespoke algorithm for generating triangle fans. Because fans are a distinct topology, with some geometry they are strictly better suited than triangle strips. Typically you'll run a pre-pass with this before passing to another algorithm (NvTriStrip usually). Controls are provided to let the user select how aggressive the optimizer should go--too aggressive and you use triangle fans in cases where strips are better suited. In practice, RiiStudio tries a variety of aggressiveness levels and picks whichever yields the best results ultimately. Future work would include a unified algorithm that is capable of making better decisions about whether to emit a strip or a fan. |
