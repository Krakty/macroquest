# PlayerClient Pad Generator

Inserts anonymous padding bytes into `src/eqlib/include/eqlib/game/PlayerClient.h`
so every declared field lands at its `/*0xNNNN*/`-comment offset, satisfying
the `static_assert(offsetof(...) == N)` entries in `RegistryAsserts.h`.

## When to run

After a patchday RE pass, when:
- `RegistryAsserts.h` has been updated with new offset comments
- `PlayerClient.h` has the new field set but field SIZES don't yet match the gaps between offsets
- The build is failing with a wave of `error C2338: PlayerClient::<field> offset mismatch` errors

## How it works

1. Parse declaration lines of the form `/*0xNNNN*/ <type> <name>[<N>]?;`
2. Compute `sizeof(field_i)` using a primitive + named-compound table plus array multipliers
3. For unknown named types, infer size from the delta to the next declaration
4. Where `offset(i+1) - offset(i) > sizeof(i)`, insert anonymous padding:
   `/*0xPADOFF*/ uint8_t pad_0xPADOFF[0xSIZE]; // MQ-RE hotfix`
5. Preserves CRLF line endings (brainiac diff-tool requirement)

## Usage

```
python3 tools/playerclient-pad-generator/pad_generator.py
```

The script edits `/tmp/PlayerClient.h` -> `/tmp/PlayerClient.h.new` and writes
a log to `/tmp/pad_log.md`. Adjust `SRC` / `DST` constants in the script if
you want a different working location.

## History

First used in apr15-2026 patchday RE work — inserted 31 pads + 1 type fixup
to resolve 101 `static_assert` errors. See
`MQ-RE:eq-builds/live/2026-04-15/iteration-logs/playerclient_hotfix_pads.md`
for the full audit trail.
