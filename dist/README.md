# Release packaging

Published builds are at [samfurr/cubiboot-ui releases](https://github.com/samfurr/cubiboot-ui/releases).
Only the hardware DOL and PicoLoader payload UF2 are shipped; this repository
does not currently produce a PicoBoot UF2, ODE ISO, or in-game-reset package.

## Build and package

1. Build a clean, pinned source revision using the [hardware build instructions](../README.md#build-for-hardware), explicitly setting `DOLPHIN_PREVIEW=0`.
2. Package `cubeboot/cubeboot.dol` as `cubiboot.dol`. Do not use a preview binary or substitute the compressed `entry/entry.dol`.
3. Download the release's `release-build-tools.tar.gz` and extract it to an ignored working directory. Follow its `BUILD.md` to run the pinned official PicoLoader converter locally with firmware inclusion disabled.
4. Validate UF2 block structure, chip-family IDs, address ranges, and an exact DOL round-trip. Keep all writes at or above PicoLoader's payload address, `0x10031000`.
5. Upload the DOL, UF2, release notes, build-tools archive, and `SHA256SUMS` to a GitHub prerelease at the exact source commit. Do not include IPL dumps, ROMs, or local preview files.
6. Download the uploaded files again and verify checksums. Test on hardware before promotion, recording the scope of that test honestly.

The existing CI workflow uploads a DOL artifact; it does not automatically create
a GitHub release or build the PicoLoader UF2. Publishing is currently a separate,
manual step.

## Promoting a tested build

Keep the tested DOL/UF2 bytes and the source tag unchanged. Update the release
notes with hardware confirmation, clear the GitHub prerelease flag, and mark the
release Latest. If documentation assets change, refresh their checksums without
rebuilding the binaries. Documentation-only follow-ups can live on `main` without
moving the binary's release tag.
