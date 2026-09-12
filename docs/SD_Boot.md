# Booting Cubiboot from SD

Use this route when PicoBoot or PicoLoader already runs gekkoboot/iplboot and
loads `ipl.dol` from an SD2SP2, SD Gecko, or similar adapter. It updates the
GameCube executable on the card without reflashing the Pico.

## Install or update

1. Back up the existing `ipl.dol` and configuration to your computer.
2. Download `cubiboot.dol` from [this fork's latest release](https://github.com/samfurr/cubiboot-ui/releases/latest).
3. Rename it to `ipl.dol` and copy it to the SD root.
4. Download the [Swiss DOL](https://github.com/emukidid/swiss-gc/releases/latest), rename it to `swiss-gc.dol`, and place it at the SD root. If your old `ipl.dol` was Swiss, you can use that backed-up Swiss file instead.
5. Safely eject the card, return it to the console, and power on without holding a boot-selection button.

The required root files are `ipl.dol` and `swiss-gc.dol`. Games can remain in
their existing folders. Swiss is required for game/program launching and appears
in the menu as **Settings**, with a gear icon, sorted last.

Configuration is optional: this fork reads `/config.ini` with a `[cubeboot]`
section, not the legacy `cubeboot.ini`. See [settings](settings.md). No
`fallback.bin` is provided or required by this installation procedure.

## Troubleshooting and rollback

- Check the actual names and locations of `ipl.dol` and `swiss-gc.dol`; avoid accidentally adding two extensions when renaming.
- Confirm the SD adapter and card work with the previously installed loader.
- Restore the backed-up `ipl.dol` to undo the update.
- No reformat is required. FAT32 enumeration is an inherited slow path; back up all data before deliberately changing a card's filesystem.
- Do not use the PicoLoader UF2 to fix an SD-boot problem on a PicoBoot installation.

See [gekkoboot's documentation](https://github.com/redolution/gekkoboot) for its
boot selection and fallback behavior. An existing embedded PicoLoader payload
uses the separate [USB update guide](RP2040_Boot.md).
