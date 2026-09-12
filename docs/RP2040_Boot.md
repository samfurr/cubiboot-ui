# Updating Cubiboot on PicoLoader

This guide is for an **existing PicoLoader installation**. A Pico appearing as a
USB drive in BOOTSEL mode does not by itself identify which GameCube mod is installed.
PicoBoot and PicoLoader have different wiring and incompatible firmware.

Our `cubiboot_picoloader.uf2` is a payload-only update, matching upstream
Cubiboot's update format. It keeps the installed PicoLoader firmware. It is not
for a blank Pico or a Pico wired for PicoBoot; for PicoBoot/gekkoboot, use the
[SD installation](SD_Boot.md) instead.

## Flash the update

1. Download `cubiboot_picoloader.uf2` from [v0.3.0-ui.2](https://github.com/samfurr/cubiboot-ui/releases/tag/v0.3.0-ui.2), initially a prerelease pending hardware confirmation, or use the previous hardware-tested [v0.3.0-ui.1](https://github.com/samfurr/cubiboot-ui/releases/tag/v0.3.0-ui.1). Keep your previous working UF2 for rollback.
2. Turn the GameCube **off**. Before connecting USB, confirm the installation follows the [official PicoLoader hardware guide](https://github.com/makeo/PicoLoader/wiki/2.1.-Normal-Installation), including power-isolation/diode wiring. Do not guess for a custom or unknown installation.
3. Hold **BOOTSEL** while connecting a USB data cable to your computer. Release the button after the USB drive appears (`RPI-RP2` on RP2040, `RP2350` on RP2350).
4. Copy the new UF2 to that drive. Wait until the copy completes and the drive disappears automatically.
5. Disconnect USB, then power on the GameCube.

No flash-nuke, full erase, or SD formatting is needed. Do not rename a `.dol` to
`.uf2`: they are different formats. Follow the [official software setup](https://github.com/makeo/PicoLoader/wiki/3.-Software-Installation)
to install PicoLoader firmware first if the board is blank.

## SD files

Keep `swiss-gc.dol` at the SD root. Cubiboot needs Swiss to launch games and
programs; the menu displays that file as **Settings**, with a gear icon, last
in the list. Your games and optional [`config.ini`](settings.md) stay on the SD.
This embedded-payload route does not require an `ipl.dol` on the card.

## Verification and rollback

The maintainer reports successful operation of **v0.3.0-ui.1** on a real GameCube
with PicoLoader. **v0.3.0-ui.2** has Dolphin UI and host regression coverage, but
its real-hardware smoke test is still pending. Check navigation, C-stick tilt
and recentering, long titles, Settings/Swiss, and game launching after updating.
Also check scrolling if your library extends beyond the three visible rows.
These checks are not an exhaustive console/adapter/IPL/reset compatibility test.

If the update fails, turn the GameCube off and repeat the BOOTSEL copy procedure
with your previous known-working Cubiboot PicoLoader UF2. Keep installed firmware
and SD contents intact. For an SD-loaded setup, restore the backed-up `ipl.dol`
instead; do not flash this UF2 to a PicoBoot installation.
