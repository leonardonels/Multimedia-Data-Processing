# Multimedia Data Processing

Exercises, lab assignments and past-exam solutions for the **Multimedia Data
Processing** course (Master's degree). The repository is a personal reference:
a collection of small, self-contained C/C++ programs that read, parse, compress,
decompress and transform binary file formats — text, images, audio and video.

It is meant to be read as much as run. Most programs implement a real-world
format or coding scheme (Huffman, LZ, PackBits, QOI, QOA, BMP, TIFF, WebP,
YUV4MPEG2, FIT, BitTorrent, …) from the specification, so the same handful of
patterns — bit I/O, magic numbers, entropy estimation, encoder/decoder
pairs — recur everywhere. Learn them once and every exercise reads the same way.

---

## Repository layout

```
.
├── .gitignore
├── README.md          ← this file
├── .vscode/           ← g++ C++23 build task (see "Building & running")
└── src/
    ├── data/          ← C++ fundamentals, entropy, generic coding & compression, file-format parsing
    ├── images/        ← raster image formats and image operations
    ├── audio/         ← audio coding / transforms
    └── video/         ← video container parsing
```

Each project lives in its own folder and is **independent**: one (occasionally a
few) `.cpp`/`.h` source files, the format specification (PDF/HTML), and sample
input/output data. There is no top-level build system — you compile one program
at a time (see below).

> **Why a single `src/` with four sub-folders?** It keeps the repository root to
> three meaningful entries (`src/`, `.gitignore`, `README.md`) and makes the
> course's four domains the first thing you see. The trade-off is one extra path
> segment when you open a file; in exchange, related exercises sit next to each
> other and the root stops being a wall of 30 folders. The VS Code build task is
> path-independent (it builds whatever file is active), so nothing in the
> workflow changes after the move.

---

## Index of exercises

### `src/data/` — fundamentals, entropy & generic coding

| Project | Topic |
|---|---|
| [read_11](src/data/read_11/) | Read 32-bit integers from text, write them as raw little-endian binary |
| [read32](src/data/read32/) | Variant: text → binary integer conversion |
| [write_11](src/data/write_11/) | Binary → text round-trip of 32-bit integers |
| [wrtite_32](src/data/wrtite_32/) | Variant of the binary↔text integer exercise |
| [sort_int](src/data/sort_int/) | The "sort integers" exercise in many flavours: C `qsort`, struct-based vector, lambdas, namespaces, streams, references — a tour of C→C++ idioms |
| [frequencies](src/data/frequencies/) | Byte-frequency counting and timing experiments over ranges/iterators |
| [prof_freq](src/data/prof_freq/) | Professor's reference frequency counter |
| [Huffman](src/data/Huffman/) | Huffman coding in four progressive versions (`huffman1..4`): plain → canonical codes, with `BitReader`/`BitWriter` |
| [HuffmanStringTable](src/data/HuffmanStringTable/) | Reusable canonical-Huffman string table (`hufstr`) + a table generator |
| [elias](src/data/elias/) | Elias universal codes (gamma/…) for integer streams |
| [packbits](src/data/packbits/) | Apple PackBits RLE encoder/decoder (`c`/`d`), plus an extension |
| [LZ77](src/data/LZ77/) | LZ77 sliding-window decoder |
| [LZ78](src/data/LZ78/) | LZ78 dictionary encoder |
| [LZ4](src/data/LZ4/) | LZ4 frame-format decompressor (magic `0x184C2103`) |
| [LZVN](src/data/LZVN/) | Apple LZVN decoder, driven by a 256-entry opcode table |
| [base64_decode](src/data/base64_decode/) | Base64 decoding (`base64.h`/`.cpp`) over a set of test strings |
| [fit](src/data/fit/) | Garmin **FIT** file dumper with header/file CRC-16 verification |
| [bittorrent](src/data/bittorrent/) | `.torrent` (bencode) parser/dumper — exam exercise |

### `src/images/` — raster formats & image operations

| Project | Topic |
|---|---|
| [pbm](src/images/pbm/) | PBM (portable bitmap, 1-bit) reader → `BinaryImage`/`Image` |
| [pgm](src/images/pgm/) | 16-bit PGM I/O (`pgm16`) on top of a generic `mat<T>` matrix |
| [pam](src/images/pam/) | PAM reading + image transforms (mirror/reverse) |
| [bmp](src/images/bmp/) | BMP (1/4/8/24-bit + palettes) → PAM converter |
| [bayer](src/images/bayer/) | Bayer-pattern demosaicing: raw PGM → gray/green/interpolated PPM |
| [Tiff](src/images/Tiff/) | TIFF IFD parser |
| [webp](src/images/webp/) | Lossless WebP (VP8L) → PAM decoder, with Huffman + distance tables |
| [QOI](src/images/QOI/) | QOI image decoder → PAM (RGBA), implementing all QOI ops (RGB/RGBA/INDEX/DIFF/LUMA/RUN) |

### `src/audio/` — audio coding & transforms

| Project | Topic |
|---|---|
| [QOA](src/audio/QOA/) | QOA ("Quite OK Audio") decoder → WAV (LMS predictor, dequant tables) |
| [MDCT](src/audio/MDCT/) | Modified DCT on 16-bit raw audio: quantization, entropy measurement, reconstruction + error signal |

### `src/video/` — video containers

| Project | Topic |
|---|---|
| [Yuv4Mpeg2](src/video/Yuv4Mpeg2/) | YUV4MPEG2 reader: parses header + frames, converts YCbCr 4:2:0 → RGB (`y4m_color`/`y4m_gray`) |

> **Note:** `Yuv4Mpeg2` `#include`s `types.h` and `utils.h`, which are not in the
> repo — it won't compile standalone until those helpers (a `vec3b`/`mat`
> definition and the I/O utilities) are supplied.

---

## Building & running

There is **no global build** — each program is compiled on its own. Two paths
exist because the code was developed on both Linux and Visual Studio:

**Linux / VS Code (primary).** `.vscode/tasks.json` defines a build task that
compiles the *currently open* file:

```bash
g++ -std=c++23 -DDEBUG -g <file>.cpp -o <file>
```

Open any `.cpp`, press the build shortcut (or `F5` to debug). The binary lands
next to the source. To build by hand:

```bash
cd src/<category>/<project>
g++ -std=c++23 -O2 main.cpp -o main      # drop -DDEBUG for a release-style build
./main <args>
```

Some projects span multiple translation units (e.g. `sort_int`,
`HuffmanStringTable`) — compile the listed `.cpp` files together.

**Windows / Visual Studio.** Several folders ship `.sln` + `.vcxproj` files;
open the solution and build as usual. `_CRT_SECURE_NO_WARNINGS` is defined where
the C standard library is used.

### Command-line convention

Most tools follow one of two shapes:

```
prog <input> <output>            # one-way transform / dump
prog [c|d] <input> <output>      # c = compress/encode, d = decompress/decode
```

Wrong argument counts and I/O failures terminate via small non-zero `exit`
codes (or an `error()`/`syntax()` helper that prints usage and exits).

### The `DEBUG` argv trick

Many `main()`s start with:

```cpp
int main(int argc, char* argv[]) {
#ifdef DEBUG
    char* d_argv[] = { (char*)"./prog", (char*)"input", (char*)"output", nullptr };
    argv = d_argv; argc = 3;
#endif
    if (argc != 3) exit(1);
    ...
}
```

Because the VS Code task passes `-DDEBUG`, you can hit run/debug with **no
arguments** and the program uses hard-coded paths. A release build (no `-DDEBUG`)
requires real CLI arguments. When you reuse a program, **check and update those
hard-coded paths** (a few still point at old `~/Downloads/...` locations).

---

## Common patterns & structures

These recur across nearly every exercise; recognising them is most of the course.

### 1. Bit I/O — `BitReader` / `BitWriter`

Codecs that aren't byte-aligned (Huffman, Elias, LZ78, WebP, LZ4) carry a tiny
bit-buffer class. Two conventions appear, differing only in **bit order**:

```cpp
class BitWriter {
    ostream& os_; uint64_t buffer_ = 0; size_t bits_ = 0;
public:
    void write_bits(uint32_t val, size_t n = 1) {
        buffer_ |= (uint64_t)(val & ((1ull<<n)-1)) << bits_;   // LSB-first here
        bits_ += n;
        while (bits_ >= 8) { os_.put(char(buffer_ & 0xFF)); buffer_ >>= 8; bits_ -= 8; }
    }
    void flush() { if (bits_) { os_.put(char(buffer_ & 0xFF)); buffer_ = 0; bits_ = 0; } }
    ~BitWriter() { flush(); }   // flushing in the destructor is the key idiom
};
```

`BitReader::read_bits(n)` mirrors it: pull whole bytes into a buffer until it
holds ≥ `n` bits, then mask off `n`. **Watch the bit order** — MSB-first
(Huffman codes, big-endian formats) vs LSB-first (LZ4/DEFLATE-style) — getting
it backwards is the classic bug.

### 2. Byte-level binary I/O & endianness

Raw `std::ifstream`/`std::ofstream` opened with `std::ios::binary`, then
`is.get()` / `os.put()` one byte at a time. Multi-byte integers are assembled
explicitly so endianness is unambiguous:

```cpp
uint32_t read_be32(istream& is) {                 // big-endian
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) v = (v << 8) | (is.get() & 0xFF);
    return v;
}
// little-endian: v |= (is.get() & 0xFF) << (8*i);
```

Magic numbers are stored as `uint32_t` constants in the file's byte order, e.g.
LZ4 `0x184C2103`, WebP `RIFF`/`WEBP`/`VP8L`, QOA `"qoaf"`, Huffman `"HUFFMAN4"`.
Verifying the magic number is always the first decode step.

### 3. The `error` / `syntax` / `check_open` helpers

The "Visual-Studio-era" files share three free functions:

```cpp
void error(const std::string& m)  { std::cout << m << '\n'; exit(EXIT_FAILURE); }
void syntax()                     { error("SYNTAX:\nprog [c|d] <in> <out>"); }
template<class T>
void check_open(const T& s, const std::string& f) { if (!s) error("cannot open " + f); }
```

The newer files instead `exit(n)` with bare numeric codes. Both styles are fine;
pick one per program and stay consistent.

### 4. Encoder/decoder as a functor class

Compression exercises wrap state (streams, buffers, mode) in a class with
`operator()`:

```cpp
class packbits_encoder {
    std::istream& is_; std::ostream& os_; /* run/copy state */
public:
    packbits_encoder(std::istream& is, std::ostream& os) : is_(is), os_(os) {}
    void operator()();   // does the whole encode
};
// main: dispatch on argv[1] == "c" / "d"
```

Streams are held **by reference** and opened in `main`, so the class never owns
files.

### 5. `mat<T>` — the image matrix

Image work uses a header-only row-major matrix ([src/images/pgm/mat.h](src/images/pgm/mat.h)):

```cpp
template<class T> class mat {
    int rows_, cols_; std::vector<T> data_;
public:
    T& operator()(int r, int c);            // bounds-checked with assert
    char* rawdata(); int rawsize();         // for bulk binary read/write
    auto begin(); auto end();               // range-for over pixels
};
```

`rawdata()`/`rawsize()` let you `is.read(m.rawdata(), m.rawsize())` a whole
plane at once; `begin()`/`end()` give you `for (auto& px : m)`.

### 6. Entropy estimation

The recurring "how compressible is this?" measurement — count symbols into a
map, then `H = −Σ pᵢ·log₂ pᵢ` (bits/symbol):

```cpp
std::unordered_map<int,size_t> hist;
for (...) hist[sym]++;
double H = 0;
for (auto [sym,n] : hist) { double p = double(n)/total; H -= p*std::log2(p); }
```

Used to compare raw vs quantized vs transformed signals (see `MDCT`,
`frequencies`).

### 7. Modern C++ in use

`#include <ranges>` with `std::ranges::iota_view(0, n)` for index loops,
structured bindings (`for (auto [k,v] : map)`), `std::clamp`, `<bit>`,
`std::format`, `string_view`, `constexpr` lookup tables (LZVN's 256-entry opcode
table, WebP's distance table). Target is **C++23**. `using namespace std;`
appears in the newer files and `std::`-qualification in the older ones — both
exist in the tree.

---

## A working pipeline for writing a codec

A repeatable recipe that fits almost every exercise here:

1. **Read the spec first.** The PDF/HTML next to the source is the source of
   truth. Note: magic number, header fields & their byte order, the chunk/op
   encoding, and how the stream ends (explicit EOF marker vs sample count).
2. **Scaffold `main`.** Parse `argc`/`argv` (`[c|d] in out` or `in out`), open
   streams in `std::ios::binary`, add the `#ifdef DEBUG` argv block so you can
   run without typing paths.
3. **Define the header struct** and a `read_header` / `write_header`. Verify the
   magic number immediately and bail with a clear error on mismatch.
4. **Pick your I/O granularity.** Byte-aligned → `get`/`put` + endian helpers.
   Bit-packed → drop in `BitReader`/`BitWriter` and decide MSB- vs LSB-first up
   front.
5. **Implement decode first.** You usually have reference encoded files but not
   an encoder; decoding to a format you can view (PAM/PPM for images, WAV for
   audio, raw for samples) gives instant visual/audible verification.
6. **Round-trip / diff.** For encoders, decode your own output and compare to the
   original; for lossy paths (MDCT, quantization) write out the **error signal**
   and measure its entropy/energy.
7. **Verify against samples.** Convert to a viewable/playable format and open it;
   re-encode known inputs and `cmp` against the provided reference bytes.

---

## Good practices & gotchas (learned in this repo)

- **Open binary streams with `std::ios::binary`.** On any platform, but
  critically so the `\r\n` translation never corrupts your bytes.
- **Be explicit about endianness.** Never `read()` a multi-byte int directly into
  memory and assume layout — assemble it byte by byte. Different formats here use
  different orders.
- **Flush bit writers in the destructor.** A half-full bit buffer that never gets
  written is the most common "last byte is wrong / truncated" bug.
- **Use `uint8_t`/`int8_t` deliberately** for pixel deltas and samples — wrap-around
  (`uint8_t(prev + diff)`) is *intended* in QOI/QOA-style predictors. Mixing in a
  signed `int` mid-expression changes the result.
- **`clamp` on reconstruction.** Lossy decoders (MDCT) must `std::clamp` samples
  back into `[-32768, 32767]` before writing 16-bit output.
- **Update the `DEBUG` hard-coded paths** before reusing a program; some point at
  machine-specific locations.
- **`.gitignore` here is aggressive.** It ignores everything, re-allows files with
  an extension, then re-ignores generated/data types (`*.txt`, `*.bin`, `*.pam`,
  `*.pgm`, `*.raw`, …) so build artifacts and large test outputs stay out of git.
  A few sample inputs are force-kept via `!name` exceptions (e.g.
  `!toms-diner.qoa`, `!testcard_rgba.qoi`, `!test.raw`). If you add a new sample
  file that must be committed, add a matching `!` exception — otherwise it is
  silently ignored. Compiled binaries (no extension, e.g. `main`) are ignored by
  design.

---

## Concept map

```
bytes/bits ──▶ entropy (H = −Σ p·log₂p)  ──▶  why compression is possible
     │
     ├─ Entropy coding ........ Huffman (canonical), Elias        → src/data
     ├─ Dictionary coding ..... LZ77, LZ78, LZ4, LZVN             → src/data
     ├─ Run-length ............ PackBits                          → src/data
     ├─ Format parsing ........ Base64, FIT (+CRC), BitTorrent    → src/data
     │
     ├─ Images ................ PBM/PGM/PAM/PPM, BMP, TIFF,       → src/images
     │                          Bayer demosaic, WebP(VP8L), QOI
     ├─ Audio ................. QOA (LMS predictor), MDCT         → src/audio
     └─ Video ................. YUV4MPEG2, YCbCr→RGB              → src/video
```

The thread running through all four domains: estimate redundancy → remove it
with a predictor/transform/dictionary → entropy-code what's left → write a
well-defined container around it. Every exercise is one slice of that pipeline.
