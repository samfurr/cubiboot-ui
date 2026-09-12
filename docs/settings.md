# Settings

Cubiboot UI reads the optional **`config.ini` at the SD root**, inside a
`[cubeboot]` section. Without it the default settings are used. Keep your existing
configuration when updating; the Settings gear opens Swiss, not a configuration editor.

For example, to request a custom boot-cube color:

```ini
[cubeboot]
cube_color = 00ffff
```

Common options read by this fork include:

- `cube_color`: hexadecimal color, or `random`.
- `force_progressive`: `1` requests progressive scan; leave it unset unless your video setup supports it.
- `preboot_delay_ms`: delay before the boot animation, in milliseconds.
- `postboot_delay_ms`: delay after the boot animation, in milliseconds.

The parser also retains older options; parsing a key does not guarantee that its
legacy behavior is active in this fork. In particular, `cube_logo` and `button_*`
remain known upstream limitations. Use gekkoboot for boot-button assignments.
See [`settings.c`](../cubeboot/source/settings.c) for accepted keys and
[`main.c`](../cubeboot/source/main.c) for their patch wiring.

Older cubeboot documentation refers to `cubeboot.ini`, `boot.dol`, and
`fallback.bin`; those are not this fork's current installation instructions.
