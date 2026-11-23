# sdbench
HOS SD Benchmark

## Building

Requires devkitPro with `libnx` (Switch) and `libounce` (Switch 2) installed,
with `DEVKITPRO` set in your environment (as in the devkitPro MSYS2 shell).

```sh
make            # build both sdbench_nx.nro and sdbench_ounce.nro
make nx         # build Switch only  -> sdbench_nx.nro
make ounce      # build Switch 2 only -> sdbench_ounce.nro
make clean      # clean both platforms
```

You can also call the platform Makefiles directly with
`make -f Makefile.nx` / `make -f Makefile.ounce`.

The shared build rules live in `Makefile.common`; each platform has its own
build directory (`build_nx` / `build_ounce`) and output name so the two builds
never overwrite each other.
