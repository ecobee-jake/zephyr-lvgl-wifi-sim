# Deferred: naming the anonymous heaps in heap-monitor

**Status:** deferred as of 2026-09-11. Revisit when running on real hardware.
**Why deferred:** the most promising approach (a build-time address→name table) is
structurally blocked on native_sim but becomes viable on a real target. See
[Why hardware changes the answer](#why-hardware-changes-the-answer).

## What exists today (working, committed to the tree)

- [src/diag/heap-monitor.c](src/diag/heap-monitor.c) / [.h](src/diag/heap-monitor.h) —
  periodic heap reporter. Named sources registered via `heap_monitor_register()` /
  `heap_monitor_register_sys_heap()`, plus a discovery pass that enumerates every
  `sys_heap` in the image via `sys_heap_array_get()` and reports unclaimed ones as
  `heap[N] @0xADDR`.
- Sampling is driven by `APP_EVENT_TICK` (every 5th tick → 5 s), dispatched from the
  `ui_thread` switch in [src/main.c](src/main.c). No timer or thread of its own.
- [prj.conf](prj.conf) — `CONFIG_SYS_HEAP_RUNTIME_STATS=y`, `CONFIG_SYS_HEAP_ARRAY_SIZE=8`.
- [scripts/decode-heaps.sh](scripts/decode-heaps.sh) — host-side workaround: annotates
  `heap[N] @0xADDR` log lines with symbol names from `nm`. Tested and working.

Current output on native_sim:

```
heap_monitor: lvgl: used=7252 total=81336 (8%) free=74084 peak=8180
heap_monitor: heap[0] @0x817b264: used=22108 total=65396 (33%) free=43288 peak=29776
heap_monitor: heap[1] @0x817cb90: used=0 total=65444 (0%) free=65444 peak=0
```

`heap[0]` is `_system_heap` (kernel, `k_malloc`); `heap[1]` is `z_malloc_heap` (libc
arena, `malloc`). Both identified by hand via `nm build/zephyr/zephyr.exe`.

## The open problem

Give the firmware itself the names, so they appear on the device console and are
available to a future UI widget — not only in host-decoded logs.

## Established facts (verified, don't re-derive)

- Heaps carry **no runtime identity**. `struct sys_heap` is `{z_heap*, init_mem,
  init_bytes}` and `sys_heap_init()` populates only the first field — `init_mem`/
  `init_bytes` are garbage unless the heap came from a static initializer.
  `struct k_heap` is `{sys_heap, wait_q, lock}`, no name, and unlike `k_mem_slab` it
  has **no `obj_core`** hook, so object-core can't enumerate or label heaps.
- Every `sys_heap` self-registers from inside `sys_heap_init()` (`sys_heap_array_save()`),
  which is why discovery finds heaps nobody exposes. Overflow of that array is
  **silent** — `sys_heap_init()` discards the `-ENOMEM`; hence the monitor's
  `LOG_WRN_ONCE` when the count hits `CONFIG_SYS_HEAP_ARRAY_SIZE`.
- `CONFIG_SYMTAB` cannot help: `scripts/build/gen_symtab.py` filters to `STT_FUNC`,
  so data addresses are absent from the table.
- `extern`-ing the heaps doesn't work: `z_malloc_heap` is `static` in libc's
  `malloc.c` (unreachable at link time), and `_system_heap` — global (`D`) in
  `zephyr.elf` — is **localized to `d` in `zephyr.exe`** by the runner link.
- Compiling an address→name table into the image is self-invalidating: the table
  lands in `.data`/`.rodata` and shifts every `.bss` address after it, including the
  heaps it describes. Failure mode is silent mislabeling.
- Zephyr's own generators dodge this by reading a *prebuilt* link stage
  (`gen_symtab.py -k $<TARGET_FILE:zephyr_pre${N}>`, see zephyr `CMakeLists.txt`
  ~1476-1488), then compiling the result into the final pass.
- **Blocker on native_sim:** `build/zephyr/zephyr.elf` is `Type: REL (Relocatable
  file)`. Every Zephyr-side stage (only `zephyr_pre0` exists here) emits relocatable
  output; the `0x0817xxxx` addresses are assigned by the *host* gcc link that builds
  `zephyr.exe`. So no prebuilt stage knows a data address, and the `gen_symtab`
  pattern has nothing to read.
- Symbol **sizes** *are* final in the relocatable ELF (`nm -S zephyr.elf` →
  `malloc_arena 0x10000`, `kheap__system_heap 0x10064`, `lvgl_heap_mem 0x14000`), and
  sizes don't shift when the image layout changes.
- Reported `total` is the backing size minus footer + chunk-0 bookkeeping: observed
  deltas were 92 B (libc), 240 B (system), 584 B (lvgl) — 0.1%–0.7%, not a
  closed-form function of size.
- One address can hold several symbols: `_system_heap` shares `0x817b264` with the
  `_k_heap_list_start` section marker (that iterable section,
  `STRUCT_SECTION_FOREACH(k_heap, h)`, enumerates static `k_heap`s but carries no
  names either). `nm` zero-pads addresses; `%p` does not — both handled in
  decode-heaps.sh.

## Candidate approaches (ranked at time of deferral)

1. **Build-time table keyed on backing-array size.** Generate `{size, name}` from
   `nm -S` on `$<TARGET_FILE:zephyr_pre0>` via a `scripts/gen_heap_names.py` +
   `add_custom_command`; at runtime match `total` to the smallest generated size ≥
   `total` within ~1% slack. Immune to the address-shift cycle, works on both
   targets, names heaps you don't own. Cost: fuzzy match; the generator must detect
   and flag build-time ambiguity when two heaps are within slack of each other.
2. **Build-time address table** (`gen_symtab` pattern). Exact, but needs the two
   passes to produce byte-identical layout (fixed-size table, padded string blob),
   and is blocked on native_sim per above. **This is the one to try on hardware.**
3. **Runtime allocator probe.** Snapshot all heaps' `allocated_bytes`, call
   `k_malloc` / `malloc` / `lv_malloc`, see which heap grew, cache the
   `struct sys_heap *` → name, free. Exact, zero build machinery, target-independent.
   Limitation: only names heaps whose allocator you can call — a heap owned by an
   uncalled subsystem stays anonymous. Must run lazily (LVGL's pool is created on the
   UI thread by `lv_mem_init()`), and needs delta-uniqueness checks to survive
   concurrent allocation by other threads.
4. **Kconfig size heuristic** (match `total` against `CONFIG_*` values). Same fuzziness
   as (1) but hand-maintained. Fallback only.

## Why hardware changes the answer

On a real target the final image *is* an EXEC ELF produced by the Zephyr link, so
`$<TARGET_FILE:zephyr_pre0>` has real `.bss` addresses and approach (2) becomes
possible — the exact, non-fuzzy option. The remaining work there is making the two
link passes layout-identical, plus using the toolchain `nm`
(`arm-zephyr-eabi-nm`, not host `nm`) in both the generator and decode-heaps.sh.

Also worth re-checking on hardware, since the heap picture changes:
- `CONFIG_LV_Z_MEM_POOL_SIZE` in [prj.conf](prj.conf) is 16384 but **dead on
  native_sim** — [boards/native_sim.conf](boards/native_sim.conf) overrides it to
  81920. On hardware the 16 KB value takes effect.
- The 64 KB libc arena (`CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE`, never set by this
  project — a picolibc default) reported `used=0, peak=0`, i.e. nothing in the image
  has ever called plain `malloc`. First candidate for reclaiming RAM on a constrained
  target.
- `heap[0]`'s `peak=29776` vs `used=22108` — the kernel heap briefly held ~7.7 KB more
  than steady state, most likely during WiFi scan/connect. Worth watching, since it's
  shared by everything calling `k_malloc`.

## Suggested skills for the next agent

- `superpowers:brainstorming` — before implementing, if the choice between approaches
  (1) and (2) is still open on the hardware target.
- `mattpocock-skills:grilling` — this design was settled by grilling; the same format
  works for the remaining open decisions (slack tolerance, ambiguity handling,
  Kconfig vs hardcoded).
- `superpowers:verification-before-completion` — the failure mode of every approach
  here is *silently wrong names*, so verify against `nm` output on the actual binary
  rather than trusting the log.
- `run` — to launch the app and capture real output on the target.

## Notes

- Verify build with `./scripts/build.sh`; run with `./scripts/run.sh` (tees to
  `/tmp/zephyr.log`). Full clean rebuild: `./scripts/build-and-run.sh -p -b native_sim`.
- `decode-heaps.sh` resolves against whatever `build/zephyr/zephyr.exe` currently is —
  decoding an old log after a rebuild yields confidently wrong names. Snapshot
  `heap-syms.txt` next to any log worth keeping.



#!/usr/bin/env bash
# Annotate heap_monitor's anonymous "heap[N] @0xADDR" lines with the owning
# symbol name, e.g. "heap[0] @0x817b264" -> "_system_heap".
#
# The kernel keeps no name for a sys_heap, and the two heaps that matter here
# cannot be named from inside the firmware: z_malloc_heap is static in libc's
# malloc.c, and compiling an address->name table into the image would shift the
# very .bss addresses it records. So the names are resolved here, on the host,
# against the same binary that produced the log.
#
# Usage: scripts/decode-heaps.sh [log]        (default /tmp/zephyr.log, - for stdin)
#        ./scripts/run.sh | scripts/decode-heaps.sh -
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

# native_sim's final link is the runner's zephyr.exe - zephyr.elf is an
# intermediate whose data symbols are all still at offset 0.
EXE=${EXE:-build/zephyr/zephyr.exe}
LOG=${1:-/tmp/zephyr.log}

if [[ ! -f "$EXE" ]]; then
	echo "error: $EXE not found - run scripts/build.sh first" >&2
	exit 1
fi

if [[ "$LOG" != "-" && ! -f "$LOG" ]]; then
	echo "error: $LOG not found" >&2
	exit 1
fi

# b/B = .bss, d/D = .data: heap control objects and their backing arrays live in
# one of the two. Anything in .text is irrelevant here.
nm "$EXE" | awk '$2 ~ /^[bBdD]$/ { print $1, $3 }' |
awk -v exe="$EXE" '
	function norm(hex) {
		# %p prints 0x817b264, nm prints 0817b264 - compare without the
		# padding, and case-insensitively.
		hex = tolower(hex)
		sub(/^0x/, "", hex)
		sub(/^0+/, "", hex)
		return hex
	}

	# First file: the symbol map.
	NR == FNR {
		addr = norm($1)

		# One address can carry several symbols: _system_heap shares its
		# address with the _k_heap_list_start section marker. Prefer the
		# real object over linker bookkeeping.
		if (addr in name && name[addr] !~ /_list_(start|end)$/) {
			next
		}
		if (addr in name && $2 ~ /_list_(start|end)$/) {
			next
		}

		name[addr] = $2
		next
	}

	{
		# heap_monitor prints "heap[N] @0xADDR:" for heaps no source claimed.
		if (match($0, /heap\[[0-9]+\] @0x[0-9a-fA-F]+/)) {
			label = substr($0, RSTART, RLENGTH)
			addr = label
			sub(/^.*@/, "", addr)

			if (norm(addr) in name) {
				sub(/heap\[[0-9]+\] @0x[0-9a-fA-F]+/,
				    name[norm(addr)] " (" label ")")
			} else {
				sub(/heap\[[0-9]+\] @0x[0-9a-fA-F]+/,
				    label " <no symbol in " exe ">")
			}
		}

		print
	}
' - "$LOG"
