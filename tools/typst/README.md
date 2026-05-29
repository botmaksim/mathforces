# Local Typst compiler

MathForces no longer runs `npx -y typst` at PDF compile time.
The server searches for the compiler in this order:

1. `MATHFORCES_TYPST_BIN` environment variable.
2. `tools/typst/typst` on Linux/macOS or `tools/typst/typst.exe` on Windows.
3. A system `typst` executable available in `PATH`.

Place the official Typst CLI binary here:

```text
tools/typst/typst      # Linux/macOS
tools/typst/typst.exe  # Windows
```

The uploaded sandbox had no network access to GitHub, so the binary itself could not be downloaded into this package here. The project code is already prepared for an offline bundled compiler.
