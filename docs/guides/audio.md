# Guide: audio

Audio goes through SDL2_mixer via the `gfx_sound_*` / `gfx_music_*`
builtins, wrapped by `std.audio2d`.

## Sound effects

```npp
import std.audio2d

fn main() {
    if !audio2d_available() {
        print("Audio unavailable: " + audio2d_error())
        return
    }

    // Procedural beep — no asset file needed.
    audio2d_tone(440.0, 250, 0.5)   // 440 Hz, 250 ms, half volume

    // Loaded sound effect.
    let coin = audio2d_load("assets/coin.wav")   // handle <= 0 on failure
    audio2d_play(coin, false)                    // looped = false
    audio2d_set_volume(0.8)
    audio2d_unload(coin)
}
```

## Music

```npp
let track = audio2d_load_music("assets/theme.ogg")
if audio2d_play_music(track, true) {         // looped
    audio2d_set_music_volume(0.6)
}
audio2d_stop_music()
```

The runtime keeps a single active music slot — there are no independent
music channels yet.

## CLI beep

For non-graphical programs `std.sys` offers `sys_beep()`, which emits
the terminal bell — handy for notifications in scripts.

## Notes

- `audio2d_available()` / `audio2d_error()` report backend status.
- Volume arguments are clamped to `[0.0, 1.0]`.
- Audio needs the SDL2 runtime; programs built with the built-in native
  backend subset may need `--backend llvm` for full support.
