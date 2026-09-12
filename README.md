# cubiboot

This is a fork of [cubeboot](https://github.com/OffBroadway/cubeboot) by [TeamOffBroadway](https://github.com/OffBroadway) with support for SD2SP2, SD Gecko or similar SD adapters.

If you have questions regarding this fork you can join the [Discord server](https://discord.gg/YtA9aU3BKZ)!

> [!IMPORTANT]
> Format your SD card using exFat (Not FAT32).\
> Currently loading of files is very slow when using FAT32 formatted SD cards.

## Local Dolphin UI preview

The menu can be built in a local preview mode that loads the configured GameCube IPL from Dolphin and replaces SD-card enumeration with representative folders, DOL files, and synthetic 96x32 game banners. This is intended for iterating on `patches/source/menu.c`, `patches/source/grid.c`, `patches/source/grid.h`, and the assets in `patches/data` without copying each build to a GameCube.

Preview mode does not use or launch `swiss-gc.dol`, and it does not validate the real SD, DVD, or boot paths. Continue to test those paths on hardware before release.

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

Preview builds are cleaned first so their conditional objects cannot leak into a hardware build. For a production build, continue to use the CI-style clean build:

```sh
make -C entry clean
make -C entry
```

## Installation - [PicoLoader](https://github.com/makeo/PicoLoader)
1. Download the [```cubiboot_picoloader.uf2```](https://github.com/makeo/cubiboot/releases/latest/download/cubiboot_picoloader.uf2) file
2. Hold down the button on the RP Pico whilst plugging it into your PC
3. Copy the .uf2 file to the USB drive
4. Download the [latest Swiss](https://github.com/emukidid/swiss-gc/releases/latest) dol
5. Rename the Swiss dol to ```swiss-gc.dol``` and place it on your SD card

## Installation - [PicoLoader](https://github.com/makeo/PicoLoader)/[PicoBoot](https://github.com/webhdx/PicoBoot) with gekkoboot payload
1. Download the [```cubiboot.dol```](https://github.com/makeo/cubiboot/releases/latest/download/cubiboot.dol)
2. Rename it to ```ipl.dol```
3. Copy the ```ipl.dol``` onto your SD card
4. Download the [latest Swiss](https://github.com/emukidid/swiss-gc/releases/latest) dol
5. Rename the Swiss dol to ```swiss-gc.dol``` and place it on your SD card

## Using In-Game Reset
1. Download [```EXTRACT_TO_ROOT.zip```](https://github.com/makeo/cubiboot/releases/latest/download/EXTRACT_TO_ROOT.zip)
2. Extract the contents to the root of the SD card
3. Pressing Z + A + START whilst in a game brings you back to the cubiboot menu

## Other ODEs (e.g. GC Loader)
Download the [```cubiboot.iso```](https://github.com/makeo/cubiboot/releases/latest/download/cubiboot.iso) and use it as appropriate for your ODE.\
Files and the config have to be stored on a separate SD2SP2, SD Gecko or similar SD adapter.\
ODEs besides PicoLoader are not supported, and issues specific to these devices might not be fixed.

## Known Bugs
- loading of files is very slow when using FAT32
- cube_logo option does not work
- button_* options to not work (use gekkoboot for this functionality instead)
- no PicoBoot uf2

## Special Thanks
- [TeamOffBroadway](https://github.com/OffBroadway) for creating cubeboot
- [Extrems](https://github.com/Extrems), [emukidid](https://github.com/emukidid) and everyone involved in creating Swiss

## Acknowledgements
- [cubeboot](https://github.com/OffBroadway/cubeboot) (GPL-2.0)
- [apploader](https://github.com/makeo/cubeboot-tools) (GPL-2.0)
- [packer](https://github.com/emukidid/swiss-gc/tree/master/cube/packer) for apploader.img (GPL-2.0)
- For more, see [CREDIT.md](https://github.com/makeo/cubiboot/blob/main/CREDIT.md)
