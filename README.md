# Cubiboot UI

A GameCube-style game selector built on [makeo/cubiboot](https://github.com/makeo/cubiboot), itself a fork of [TeamOffBroadway's cubeboot](https://github.com/OffBroadway/cubeboot). Browse games from SD2SP2, SD Gecko, or similar SD adapters in a scrolling cube grid, with a large selected-game preview and readable banners and titles.

- Four-column scrolling grid, a bobbing selected-game cube, and a stationary details panel.
- C-stick inspection tilts the large cube in pitch/yaw and returns to its resting pose when released; the main stick and D-pad navigate the grid.
- Warm-yellow selection corners, a short arrival gesture, gentle edge feedback, and an occasional idle nod give the native cube cabinet a little personality without moving its details panel.
- Wide GameCube banners keep their aspect ratio; long titles wrap and fit the panel.
- Swiss appears as **Settings**, with a gear icon, and always sorts last.
- Original IPL background and native Back prompt.
- Isolated Dolphin preview for visual development without reflashing hardware.

## Downloads

Get the [latest release](https://github.com/samfurr/cubiboot-ui/releases/latest), including checksums and build information.

| Your installation | File | Where it goes |
| --- | --- | --- |
| Existing PicoLoader, with Cubiboot stored on the Pico | [`cubiboot_picoloader.uf2`](https://github.com/samfurr/cubiboot-ui/releases/latest/download/cubiboot_picoloader.uf2) | Pico USB drive in BOOTSEL mode |
| PicoBoot or PicoLoader using gekkoboot/iplboot to load `ipl.dol` from SD | [`cubiboot.dol`](https://github.com/samfurr/cubiboot-ui/releases/latest/download/cubiboot.dol) | SD root, renamed to `ipl.dol` |

> [!WARNING]
> PicoLoader and PicoBoot are different installations; their UF2 files are not interchangeable. Our UF2 is a **PicoLoader payload-only update**: it preserves existing PicoLoader firmware and will not set up a blank board. This fork does not provide a PicoBoot UF2.

The maintainer has confirmed this release works on a real GameCube with PicoLoader. That is a hardware smoke test, not a compatibility guarantee for every console, SD adapter, or launch/reset path.

### Update an existing PicoLoader

1. Keep your previous working UF2 and back up your SD configuration.
2. Turn the GameCube **off**. Verify the installation's power-isolation/diode wiring before connecting USB; follow the [official hardware guide](https://github.com/makeo/PicoLoader/wiki/2.1.-Normal-Installation) for an unknown or custom installation.
3. Hold **BOOTSEL** while connecting the Pico to your computer. Release it when the USB drive appears.
4. Copy `cubiboot_picoloader.uf2` onto that drive and wait for the drive to disappear automatically.
5. Disconnect USB, then power on the GameCube.

Keep [Swiss](https://github.com/emukidid/swiss-gc/releases/latest) on your SD root as `swiss-gc.dol`; it is required for game/program launching and appears as **Settings**. Existing games and configuration stay in place. Do not erase the Pico or use a flash-nuke UF2 for this update.

See the [flashing and rollback guide](docs/RP2040_Boot.md). First-time PicoLoader installations need the [official firmware setup](https://github.com/makeo/PicoLoader/wiki/3.-Software-Installation) before our payload.

### Install through SD / gekkoboot

1. Back up the SD card's existing `ipl.dol` to your computer.
2. Download `cubiboot.dol`, rename it to `ipl.dol`, and copy it to the SD root.
3. Put the Swiss DOL at the SD root as `swiss-gc.dol`.
4. Safely eject the card, return it to the GameCube, and power on.

This route does not require reflashing the Pico. See the [SD boot guide](docs/SD_Boot.md).

### Configuration and storage

The optional settings file is **`config.ini` at the SD root**, with a `[cubeboot]` section; see [configuration](docs/settings.md). A configuration file is not required for the default menu. The Settings gear launches Swiss; it is not an editor for this file.

No SD reformatting is required to update. The upstream fork reports slow FAT32 enumeration and recommends exFAT when preparing a card; back up its contents before changing filesystems. Nintendo IPL dumps, games, and Swiss are not included in our downloads.

## Local Dolphin UI preview

The menu can be built in a local preview mode that loads the configured GameCube IPL from Dolphin and replaces SD-card enumeration with representative folders, DOL files, and synthetic 96x32 game banners. This is intended for iterating on `patches/source/menu.c`, `patches/source/grid.c`, `patches/source/grid.h`, and the assets in `patches/data` without copying each build to a GameCube.

Preview mode does not use or launch `swiss-gc.dol`, and it does not validate the real SD, DVD, or boot paths. Continue to test those paths on hardware before release.

`swiss-gc.dol` appears in the grid as **Settings**, with a gear icon (including a mock entry in preview mode), and always sorts after the other entries. Its filename and launch behavior are unchanged; there is no separate Start shortcut in the grid. To regenerate the bundled gear texture, run `python3 scripts/generate-settings-icon.py`.

### One-time setup

1. Install [Dolphin](https://dolphin-emu.org/download/) and [devkitPro pacman](https://github.com/devkitPro/pacman/releases/latest).
2. Install the GameCube toolchain and port libraries:

   ```sh
   sudo dkp-pacman -S gamecube-dev ppc-libmad ppc-zlib
   ```

3. Install the libogc2 and libfat variants used by this repository's CI:

   ```sh
   git clone https://github.com/extremscorner/libogc2.git /tmp/libogc2
   env DEVKITPRO=/opt/devkitpro DEVKITPPC=/opt/devkitpro/devkitPPC make -C /tmp/libogc2
   sudo env DEVKITPRO=/opt/devkitpro DEVKITPPC=/opt/devkitpro/devkitPPC make -C /tmp/libogc2 install

   git clone https://github.com/extremscorner/libfat.git /tmp/libfat
   env DEVKITPRO=/opt/devkitpro DEVKITPPC=/opt/devkitpro/devkitPPC make -C /tmp/libfat ogc-release
   sudo env DEVKITPRO=/opt/devkitpro DEVKITPPC=/opt/devkitpro/devkitPPC make -C /tmp/libfat/libogc2 install
   ```

4. Put a legally obtained GameCube IPL at `~/Desktop/IPL.bin`. The launcher copies it into a disposable Dolphin profile outside the repository; it is never added to git. Set `IPL_BIN` to use another path, or `DOLPHIN_IPL_REGION` for a region other than the default `USA`.

### Edit, build, preview

Quit any currently running cubiboot preview, then run:

```sh
make preview
```

`make preview-build` only builds the preview DOL, while `make preview-run` builds and launches it. The build uses `DOLPHIN_PREVIEW=1`, skips the boot animation, and creates its pinned Python helper environment under the ignored `.venv` directory.

Dolphin's default keyboard controls use the arrow keys for the main stick, `X` for A, `Z` for B, `D` for Z, and Return for Start.

C-stick inspection uses `I`/`K` for pitch and `J`/`L` for yaw in Dolphin's default keyboard mapping. Release the keys to recenter the large cube. The small grid cubes and details panel do not rotate.

The details panel fits titles using the IPL font's glyph widths: one line for short names, two balanced lines for longer ones, then a small size reduction and an ellipsis only when necessary. It updates immediately when selection changes, while the cube animates independently.

The cabinet uses a 200 ms arrival, a small 120 ms selection settle, and a single quiet nod after 11 seconds of inactivity. The nod dips five degrees over 400 ms and eases back over 800 ms, so it reads distinctly from the idle bob. C-stick input cancels decorative rotation immediately. Reaching a boundary gives one short bump and native error sound, including when reached through held navigation; holding against that edge stays quiet. Launch alignment runs during the existing transition and adds no boot delay.

`make test` runs the host-side title-layout, menu-input, IPL input-hook, and menu-motion regression tests without launching Dolphin. New motion is checked at both 50 and 60 Hz; real SD access and boot behavior still require a hardware smoke test.

Controller input is captured immediately after the IPL's normal `PADClamp` call, before its menu combines both sticks into D-pad presses and clears the analog axes. This reuses the existing poll and preserves native controls outside the loader. A guarded instruction-pattern match locates the hook; if it cannot be identified uniquely, the loader falls back to native navigation without C-stick inspection.

### Build for hardware

Do not flash or install the output of `make preview`. After the dependencies above are installed, build from the repository root with a clean, non-preview configuration:

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC="$DEVKITPRO/devkitPPC"
make preview-python-deps
export PATH="$PWD/.venv/bin:$PATH"
mkdir -p .cache/go-build
export GOCACHE="$PWD/.cache/go-build"
make test
make -C entry clean
make -C entry DOLPHIN_PREVIEW=0
```

The standalone output is `cubeboot/cubeboot.dol`, published as `cubiboot.dol`. The compressed `entry/entry.dol` is a different artifact. UF2 packaging uses the pinned official PicoLoader converter supplied in the release's build-tools archive; see [release packaging](dist/README.md). Renaming a DOL to UF2 is not sufficient.

## Limitations and upstream extras

- FAT32 enumeration remains slow.
- The upstream `cube_logo` and `button_*` settings remain unsupported/broken; use gekkoboot for boot-button assignments.
- We do not ship a PicoBoot UF2, an ODE ISO, or an in-game-reset package in this release.
- Upstream's [v0.3 release](https://github.com/makeo/cubiboot/releases/tag/v0.3) has ISO/reset extras, but these have not been rebuilt or validated for this UI fork and may contain the upstream menu. Do not treat them as this release's UI assets.
- The hardware smoke test does not establish compatibility across all IPL revisions or independently verify every game, Swiss launch, or in-game-reset combination.

## Support and history

Report issues for this UI fork in [this repository](https://github.com/samfurr/cubiboot-ui/issues). The [upstream Discord](https://discord.gg/YtA9aU3BKZ) is a separate community. See [CHANGELOG.md](CHANGELOG.md) for the UI release and inherited project history.

## Special Thanks

- [makeo](https://github.com/makeo) for Cubiboot and PicoLoader.
- [TeamOffBroadway](https://github.com/OffBroadway) for creating cubeboot
- [Extrems](https://github.com/Extrems), [emukidid](https://github.com/emukidid) and everyone involved in creating Swiss

## Acknowledgements

- [cubeboot](https://github.com/OffBroadway/cubeboot) (GPL-2.0)
- [apploader](https://github.com/makeo/cubeboot-tools) (GPL-2.0)
- [packer](https://github.com/emukidid/swiss-gc/tree/master/cube/packer) for apploader.img (GPL-2.0)
- For more, see [CREDIT.md](CREDIT.md).
