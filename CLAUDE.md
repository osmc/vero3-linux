# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

> **Editing this file:** CLAUDE.md is maintained as a single, isolated commit kept at the tip of the branch. It must **never** be bundled into a code/feature commit, and you should **not** create your own commit for it — make the edit and leave it uncommitted in the working tree for the maintainer to fold into that one commit (via `git rebase` + `fixup`, force-pushed). This keeps documentation changes out of code history and conflict-free across the 5.15 rebase.

## What this repo is

Linux 4.9.269 kernel fork maintained by OSMC for the **Vero V** media player (and the earlier **Vero 4K** / **Vero 4K+**). The SoC is Amlogic **SC2 (S905X4)**, ARM64. The base is Amlogic's vendor BSP on top of Google's `android-4.9-q` common kernel; the upstream is private (`gitlab.com/osmc-nda/vero5-linux`) and a `stable` remote tracks `linux.git` for cherry-picks.

Note the product/codename mismatch in source:
- `arch/arm64/boot/dts/amlogic/vero3_2g_16g.dts` → **Vero 4K**
- `arch/arm64/boot/dts/amlogic/vero3plus_2g_16g.dts` → **Vero 4K+**
- `arch/arm64/boot/dts/amlogic/vero5_2021.dts` → **Vero V**

`vero3*` and `vero5*` are internal codenames only — the shipping products are Vero 4K / Vero 4K+ / Vero V. Don't refer to anything as "Vero 3" in user-facing text or commit messages.

This tree is **heavily customised** away from the stock Amlogic BSP. The three big themes are:

1. **Dolby Vision** — FEL/MEL, profile 5/7/8, TV-led DV, HDR10+ interop, OSD brightness/colour for P7, BL/EL scaling and merging.
2. **3D MVC / frame-packed 3D** — HSBS, HTAB, full-SBS/TAB, l/r-eye handling, MVC canvas-register fixes, 3D→2D conversion, EDID 3D-mode discovery.
3. **HD audio** — multi-channel LPCM, high-samplerate PCM, passthrough/HBR, HDMI audio init, N-value tweaks for 44.1 kHz with 4K video, channels_max=8 paths.

These changes are **not isolated to one driver** — they reach end-to-end across the media path. For DV/3D: codec → frame_sync/avsync → video_sink (`video_hw.c`, `vpp.c`) → amvecm → amdolby_vision → hdmitx. For audio: ALSA layer → `sound/soc/amlogic/` → hdmitx audio init / i2s2hdmitx / SPDIF / auge. When figuring out *why* a piece of code looks the way it does, search the OSMC-only commit range (`git log osmc-4.9-bringup-jan2023..osmc-4.9-bringup-aug2024`) — the rationale almost always lives there, not in Amlogic history.

The single active development branch is **`osmc-4.9-bringup-aug2024`**. `osmc-4.9-bringup-jan2023` is the historical PR base for diffs but is not under active development. Other branches in the repo (`DVtvled*`, `WIPDVFEL`, etc.) are stashes / experiments — don't treat them as live.

## Forward direction: 5.15 migration

A migration to Amlogic's **5.15** kernel is planned. Factor that in when making non-trivial changes:
- Prefer fixes that will map cleanly onto the 5.15 Amlogic vendor tree over ones that lean on quirks of the 4.9 BSP.
- Avoid new dependencies on 4.9-only APIs or pre-DRM/KMS plumbing when a more portable formulation exists.
- When touching DV / 3D MVC / hdmitx / audio, write commit messages that explain the *intent and the underlying behaviour* — the patch will need to be re-applied on a base where the surrounding code looks very different.

## Crash diagnostics — keep post-mortem capture working

Freezes that can't be reproduced on the bench are only debuggable because a silent hang is turned into a captured panic + auto-reboot, leaving a `dmesg-ramoops` that survives the reboot. Several pieces make that work — **don't remove them, and carry them onto 5.15**:

- **ramoops region + scrambler-key preserve.** `ramoops@0x08400000` (1 MB) is a no-map `reserved-memory` node in DT, and `scrambler_ramoops_init()` (`drivers/amlogic/debug/debug_scrambler_ramoops.c`, DT prop `amlogic, ddr-scrambler-preserve`) keeps the DDR scramble key across a reset so the region stays readable. ramoops survives a **warm reset** (kernel panic / watchdog) but **not a power cycle** — DRAM contents are lost. The whole strategy is therefore: turn a hang into a warm-reset reboot *before the user pulls power*.
- **Lockup → panic, driven from userland.** `kernel.hardlockup_panic=1`, `kernel.softlockup_panic=1`, `kernel.panic_on_oops=1` (+ `kernel.panic=5` for the reboot timeout) live in OSMC's `package/vero5-device-osmc/files/etc/sysctl.d/101-osmc-device.conf`. Kept as sysctls, *not* baked into the kernel config, so they carry across the 5.15 migration with no defconfig work. The hard-lockup knob is the key catcher: a core wedged with IRQs off (the shape of the rtl8822CS rx freeze) produces no oops, so only the lockup detector can turn it into a reboot.
- **The Amlogic cross-CPU detector.** ARM64 4.9 has no real NMI, so `CONFIG_HARDLOCKUP_DETECTOR_OTHER_CPU=y` (`kernel/watchdog_hld.c`) is what spots an IRQs-off spin — a peer CPU checks it from the timer hrtimer and calls `panic()` gated on the standard `hardlockup_panic` global. **This symbol must stay enabled on 5.15** or `kernel.hardlockup_panic` becomes a no-op.
- **The hardware watchdog is NOT the catcher.** `mesonsc2.dtsi` sets `reset_watchdog_method=<1>` (kernel), so the kernel auto-pets meson-wdt via an hrtimer — it only fires on a total freeze where timers also stop. Don't rely on it for hang detection; the lockup detector is the mechanism.

Reading dumps: they land in **`/var/lib/systemd/pstore/`** (systemd-pstore moves them off `/sys/fs/pstore`, which is empty once userspace is up), or the grab-logs "ramoops content" section. `dmesg-ramoops-*` is the per-panic `kmsg_dump`; `console-ramoops-*` is the rolling console ring (retains multiple events).

Validating the path: `modprobe panic mode=2` injects an IRQs-off hard lockup — modes: `0` panic, `1` softlockup, `2` hardlockup, `3` hung-task (`3` is inert unless `CONFIG_DETECT_HUNG_TASK`, deliberately left off to avoid panics on slow NFS/SMB mounts). Detection takes ≈ 2× `watchdog_thresh` (~20 s) — the box looks dead, then panics; tell testers to wait ~30 s, not power-cycle. UART is on the cmdline (`console=ttyS0,921600 earlycon=aml-uart`) but the header pins require opening the case.

## rtl8822CS WiFi (Vero V) — NAPI disabled on 4.9

The Vero V WiFi driver (`drivers/osmc/wlan/rtl8822CS/`) is Realtek/Amlogic's **5.4 / Android-T** driver (upstream history: "support rtl8822cs for kernel5.4", "bringup for android t" — no 4.9 target anywhere). Its NAPI receive path assumes net-core behaviour 4.9 doesn't provide, and caused playback-freeze **soft-lockups** — a CPU wedged in `__do_softirq` (silent freeze; on WiFi only, never wired). With GRO on it surfaced first as a `dev_gro_receive` NULL-deref; with GRO off it became the softirq livelock. The Dolby Vision parser errors that show up alongside are a *symptom* (the decoder starving once rx stalls), not the cause — the freeze predates DV and reproduces on plain SDR/HDR.

Toggling the driver's NAPI sub-options only *moved* the failure: the vendor's own "fix 8822cs napi crash" is just `//#define CONFIG_RTW_NAPI_DYNAMIC`, which trades the crash for a drain-starvation regression on 4.9 (the indicate path stops scheduling the poll under `NAPI_V2`). There is no 4.9-clean driver upstream.

**Resolved on 4.9 by `CONFIG_RTW_NAPI = n`** in the rtl8822CS Makefile: the entire `#ifdef CONFIG_RTW_NAPI` block compiles out (DYNAMIC / V2 / GRO-via-napi / eager-drain) and all rx goes through the kernel's own `netif_rx`/backlog path, which 4.9 schedules and budgets natively. `CONFIG_RTW_GRO = n` is kept as defence. Minor peak-throughput cost, nil for streaming.

**On 5.15: revert `NAPI=n`** — re-take the vendor driver's NAPI path fresh, where its assumptions hold.

## Code layout that matters

Most of what's special here is **not** in standard kernel directories:

- `drivers/amlogic/` — Amlogic vendor BSP. Huge tree; the parts touched most often are under `drivers/amlogic/media/`:
  - `enhancement/amdolby_vision/amdolby_vision.c` — Dolby Vision processing (FEL/MEL/profile 5/7/8 paths). The recent DV/FEL commits land here.
  - `enhancement/amvecm/` — video enhancement / color management (HDR10+, gamut, tone-mapping). Gated by `CONFIG_DEBUG_OSMC` in places.
  - `video_sink/video_hw.c`, `video.c`, `vpp.c` — final video pipeline / VPP / scaling (HSC/VSC), the composition path feeding HDMI.
  - `vout/` — output / HDMI mode programming; full-range YUV support for VESA modes lives here.
  - `frame_sync/`, `avsync/` — A/V timing; `audio/` interactions (N-value tweaks for 44.1kHz with 4K).
- `drivers/osmc/` — OSMC's own additions. Subdirs:
  - `gpu/` — Mali driver bundles (mali, mali-mg, memory_group_manager)
  - `wlan/` — bcmdhd + Realtek rtl8822CS + a `scan` helper
  - `videoenhancement/` — userspace TA bridge (`CONFIG_OSMC_VIDEOENHANCEMENT=y`)
  - `panic/` — fault-injection module (`CONFIG_OSMC_PANIC=m`); `modprobe panic mode=N` injects a panic / soft-lockup / hard-lockup / hung-task to validate the crash-capture path (see "Crash diagnostics")
  - `secureosmc/` — TEE shim (`tee_core.c`, `tee_shm.c`, `tee_data_pipe.c`) used by the closed OSMC.dovi.ko Dolby Vision blob
- `drivers/amlogic/wifi/wifi_dt.c` — Amlogic wifi power/DT glue (separate from the bcmdhd driver under `drivers/osmc/wlan/`)
- `net/wireguard/` — out-of-tree WireGuard (Zinc) backport; aarch64 `.S` files there appear untracked because they're generated.
- `drivers/amlogic/mmc/` — Amlogic SD/eMMC host controller (`aml_sd_emmc.c`, `aml_sd_emmc_v3.c` for the SC2 v3 controller; `emmc_partitions.c` for the `/sys/class/aml_store/` partition nodes). Standard MMC core lives in `drivers/mmc/core/` (`mmc.c` parses EXT_CSD incl. the eMMC-5.0 health bytes; `debugfs.c` exposes the raw `ext_csd`).

### eMMC sysfs / health layout (Vero V)

The Amlogic driver names its MMC hosts by **function, not `mmcN`**: `emmc`, `sd`, `sdio` (platform devices `fe08c000.emmc`, `fe08a000.sd`, `fe088000.sdio`). So the eMMC card is `emmc:0001`, and globs like `/sys/class/mmc_host/mmc*/mmc*:*` or paths like `/sys/kernel/debug/mmc0/...` find **nothing** — they're under the `emmc` name. The reliable, naming-proof paths:
- `/sys/block/mmcblk0/device/{rev,pre_eol_info,life_time,name,...}` — eMMC is `mmcblk0` (owns `mmcblk0boot0/boot1/rpmb`)
- equivalently `/sys/class/mmc_host/emmc/emmc:0001/...`; raw dump at `/sys/kernel/debug/emmc/emmc:0001/ext_csd`

The standard health nodes (`pre_eol_info`, `life_time`) **are** real reads (gated on EXT_CSD rev ≥ 7 in `mmc_decode_ext_csd()`, so a non-zero value is genuine, not a default) — cross-checkable with `mmc extcsd read /dev/mmcblk0` (ioctl; `mmc-utils` is **not** installed in OSMC by default). They only carry a coarse vendor wear estimate, never bus errors: the controller silently retries/re-tunes CRC/timeout faults so recoverable errors never reach the block layer or dmesg. The `emmc_errors` sysfs counter on `fe08c000.emmc` (`struct amlsd_host` counters, surfaced in `aml_sd_emmc.c`) exists for that gap.

## Conventions / hazards specific to this tree

- **Don't reformat or `checkpatch`-clean Amlogic vendor code.** It diverges heavily from upstream style; reflowing it makes future rebases against Amlogic drops painful. Touch only what the change requires.
- The video pipeline talks across `amdolby_vision` ↔ `amvecm` ↔ `video_hw`/`vpp` via shared globals and register writes — changes in one file often need matching changes in the others. Read commits like `d754885 Fixed HSC/VSC region calculation` and the DV FEL series to see the cross-file pattern before editing.
- Debug-only register dumps and verbose logs are gated behind `CONFIG_DEBUG_OSMC` (see `drivers/amlogic/media/enhancement/amvecm/Kconfig` and recent commits hiding `hdr2_debug`). Don't add unconditional dumps — guard them.
- The Dolby Vision closed module is built out-of-tree as **OSMC.dovi.ko** and binds through `drivers/osmc/secureosmc/`. Symbols exported from the in-tree code form an ABI for that blob — be careful when renaming/removing exports in `amdolby_vision` and `secureosmc/`.
- `CONFIG_LOCALVERSION_AUTO=y` is on, so the kernel version string picks up the git description. Don't expect a clean `EXTRAVERSION`.
- This kernel uses cgroup v1 explicitly (`systemd.unified_cgroup_hierarchy=0` in `CONFIG_CMDLINE`). Don't propose moving userspace assumptions to v2.

## Working with patches / commits

- Commit subjects in this tree tend to be short, area-prefixed, lowercase-ish: `VIDEO:`, `DV:`, `DV FEL:`, `AUDIO:`, `MMC:`, `Kconfig:`. Match the existing style of nearby commits rather than imposing a conventional-commits format.
- `git log osmc-4.9-bringup-jan2023..osmc-4.9-bringup-aug2024` is the right way to see what's new in the current bringup vs. the PR base.
