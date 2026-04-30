#!/usr/bin/env python3
"""
Insert anonymous padding into PlayerZoneClient class body of PlayerClient.h
so every declared field lands at its comment-stated offset.

Strategy:
- Parse lines of the form:
    /*0xNNNN*/ <typetokens...> <name>[<arraytoken>]?;
- For each consecutive pair (line_i, line_i+1):
    gap = offset_{i+1} - offset_i
    Compute sizeof(field_i). If sizeof < gap, need padding of (gap - sizeof).
- Insert a new line before line_{i+1}:
    /*0xPADOFF*/ uint8_t                  pad_0xPADOFF[0xSIZE];
- Skip inserting if the existing next line ALREADY is a pad field with
  the correct size (idempotency).
- Only process lines in the range [FIRST, LAST] (PlayerZoneClient body only).

sizeof rules:
- primitive tokens map to explicit sizes
- arrays: name[LITERAL] where LITERAL is hex/decimal or a known constant
- unknown named types: derive size from delta between this declaration's
  offset and the next declaration's offset, AND TRUST THAT AS THE TYPE'S SIZE.
- If we can't compute sizeof and the gap equals what we'd naively expect,
  skip (assume no padding needed).
"""
import re
import sys
from pathlib import Path

SRC = Path("/tmp/PlayerClient.h")
DST = Path("/tmp/PlayerClient.h.new")
LOG = Path("/tmp/pad_log.md")

# Known sizes for primitive types.
PRIM = {
    "bool": 1,
    "char": 1,
    "uint8_t": 1, "int8_t": 1, "BYTE": 1,
    "uint16_t": 2, "int16_t": 2, "WORD": 2, "short": 2, "wchar_t": 2,
    "uint32_t": 4, "int32_t": 4, "int": 4, "unsigned": 4, "DWORD": 4,
    "float": 4, "long": 4,
    "uint64_t": 8, "int64_t": 8, "QWORD": 8, "double": 8,
    "EActorType": 4,
    "EqItemGuid": 0x12,  # struct { char[0x12]; } per upstream declaration
}
# Pointer sizes (anything with '*' in type or explicit pointer types)
PTR_SIZE = 8

# Known compound types with explicit sizes
COMPOUND = {
    "CPhysicsInfo": 0x30,
    # LaunchSpellData declared at 0x44 (see struct). Upstream PZC layout reserves
    # 0x4C (8B alignment pad for following int64 HPCurrent). We size it at 0x44
    # so the pad pass inserts 8B explicit pad.
    "LaunchSpellData": 0x44,
    "CharacterPropertyHash": 0x18,
}

# Known array size constants (from EQ headers)
CONSTS = {
    "CONCURRENT_SKILLS": 2,
    "EQ_MAX_NAME": 64,
    "MAX_GROUP_ASSISTS": 1,
    "MAX_RAID_ASSISTS": 3,
    "MAX_GROUP_MARK_TARGETS": 3,
    "MAX_RAID_MARK_TARGETS": 3,
    "MAX_MOVEMENT_STATS": 20,
}


def parse_literal(s):
    s = s.strip()
    if s.startswith("0x") or s.startswith("0X"):
        return int(s, 16)
    if s.isdigit():
        return int(s, 10)
    if s in CONSTS:
        return CONSTS[s]
    return None


DECL_RE = re.compile(
    r"^(\s*)/\*(0x[0-9a-fA-F]+)\*/\s+(.*?);(\s*(?://.*)?)$"
)


def parse_decl(line):
    """Return (indent, offset_int, decl_text, trailing_comment) or None."""
    m = DECL_RE.match(line.rstrip("\n"))
    if not m:
        return None
    indent = m.group(1)
    off = int(m.group(2), 16)
    body = m.group(3).strip()
    trail = m.group(4)
    return (indent, off, body, trail)


def sizeof_decl(body, next_off=None, cur_off=None):
    """Given declaration text like 'char Title[0x80]' return size in bytes
    or None if we can't compute. If next_off + cur_off given, we can infer
    unknown struct sizes from the delta."""
    # split name/array from type
    # Find 'name' token: right-to-left find last identifier-like token.
    # Handle 'char Handle[0x20]' => type='char', name='Handle[0x20]'
    # Handle 'unsigned int**  ppUDP' => type='unsigned int**', name='ppUDP'
    # Pointer if '*' appears anywhere before the name
    # Strategy: split on whitespace, last token is name(+array), rest is type.
    toks = body.split()
    if len(toks) < 2:
        return None
    name_tok = toks[-1]
    type_toks = toks[:-1]
    type_str = " ".join(type_toks)

    # Count pointer stars in whole declaration
    stars = type_str.count("*") + name_tok.count("*")

    # Array: find '[' in name
    arr_size = 1
    if "[" in name_tok:
        # may be multi-dim; just handle single-dim for now
        m = re.search(r"\[([^\]]+)\]", name_tok)
        if m:
            n = parse_literal(m.group(1))
            if n is None:
                return None
            arr_size = n

    # Determine element size
    elem = None
    # pointer takes priority
    if stars > 0:
        elem = PTR_SIZE
    else:
        # find known primitive/compound in type_toks
        # normalize 'unsigned int' -> 'int' etc.
        tt = [t for t in type_toks if t not in ("const", "volatile", "static")]
        tt_str = " ".join(tt)
        # Map common 'unsigned int' / 'unsigned' alone
        if tt_str in ("unsigned int", "signed int", "unsigned long", "signed long"):
            elem = 4
        elif tt_str in ("unsigned char", "signed char"):
            elem = 1
        elif tt_str in ("unsigned short", "signed short"):
            elem = 2
        elif tt_str in ("unsigned long long", "signed long long", "long long"):
            elem = 8
        else:
            # Try single-token primitive
            if len(tt) == 1 and tt[0] in PRIM:
                elem = PRIM[tt[0]]
            elif len(tt) == 1 and tt[0] in COMPOUND:
                elem = COMPOUND[tt[0]]

    if elem is None:
        # Try to infer from gap to next decl
        if next_off is not None and cur_off is not None and arr_size == 1:
            gap = next_off - cur_off
            return gap  # trust the layout
        return None
    return elem * arr_size


TYPE_FIXUPS = {
    # field_name -> new_type_token (must be same width as registry's canonical)
    # AltAttack is u8 per registry at 0x5D2; upstream header has int which
    # overlaps LastPrimaryUseTime at 0x5D4. Shrink type, then natural pad pass
    # inserts 1B pad.
    "AltAttack": ("int", "uint8_t"),
}


def main():
    raw = SRC.read_bytes()
    # Detect line ending
    eol = b"\r\n" if b"\r\n" in raw[:4096] else b"\n"
    text = raw.decode("utf-8")
    src = text.splitlines(keepends=True)
    # splitlines(keepends=True) will include the CRLF. Good.

    # Apply type fixups before parsing.
    # Match '/*0xNNNN*/ <oldtype> <ws> <name>;' and swap oldtype -> newtype.
    for fname, (old_t, new_t) in TYPE_FIXUPS.items():
        pat = re.compile(
            r"^(\s*/\*0x[0-9a-fA-F]+\*/\s+)" + re.escape(old_t) + r"(\s+" + re.escape(fname) + r"\s*;.*)$"
        )
        for i, line in enumerate(src):
            m = pat.match(line.rstrip("\r\n"))
            if m:
                # Pad new_t to preserve column alignment of name
                # Replace only first match per field
                # Keep the old token width by trailing spaces
                old_len = len(old_t)
                new_tok = new_t
                if len(new_tok) < old_len:
                    new_tok = new_tok + (" " * (old_len - len(new_tok)))
                # Preserve original EOL
                orig_line = src[i]
                if orig_line.endswith("\r\n"):
                    end = "\r\n"
                elif orig_line.endswith("\n"):
                    end = "\n"
                else:
                    end = ""
                src[i] = m.group(1) + new_tok + m.group(2) + end
                print(f"Fixup: {fname} {old_t} -> {new_t} at line {i+1}", file=sys.stderr)
                break

    # Find PlayerZoneClient class region
    # We apply to ANY declaration line after 'class [[offsetcomments]] PlayerZoneClient'
    # up to its closing '};' (line 707 area).
    start_idx = None
    end_idx = None
    for i, line in enumerate(src):
        if start_idx is None and "class [[offsetcomments]] PlayerZoneClient" in line:
            start_idx = i
        elif start_idx is not None and end_idx is None and line.rstrip() == "};":
            end_idx = i
            break
    assert start_idx is not None and end_idx is not None, "PZC region not found"
    print(f"PZC region: lines {start_idx+1}..{end_idx+1}", file=sys.stderr)

    # Walk the body, collect declaration lines with their offsets.
    decls = []  # (line_index, offset, body)
    for i in range(start_idx + 1, end_idx):
        p = parse_decl(src[i])
        if p is None:
            continue
        _, off, body, _ = p
        decls.append((i, off, body))

    # For each pair, compute sizeof(decl_i) and gap.
    inserts = []  # list of (line_index_before_which_to_insert, pad_line)
    log_entries = []
    for k in range(len(decls) - 1):
        i, off, body = decls[k]
        _, next_off, _next_body = decls[k + 1]
        gap = next_off - off
        sz = sizeof_decl(body, next_off=next_off, cur_off=off)
        if sz is None:
            log_entries.append(f"- SKIP sizeof unknown: offset 0x{off:04X} body=`{body}` gap=0x{gap:X}")
            continue
        if sz > gap:
            log_entries.append(f"- OVERLAP!! offset 0x{off:04X} sz=0x{sz:X} gap=0x{gap:X} body=`{body}` next_off=0x{next_off:04X}")
            continue
        if sz < gap:
            pad_off = off + sz
            pad_size = gap - sz
            # Skip if existing next-next sibling is already a pad inserted here:
            # simplest: check if the very next decl IS a pad field we inserted.
            # We haven't inserted yet, so just check for pad_0x prefix in next body
            next_idx, _, nb = decls[k + 1]
            if nb.startswith("uint8_t") and f"pad_0x{pad_off:04X}" in nb:
                log_entries.append(f"- already padded: offset 0x{off:04X} -> 0x{next_off:04X}")
                continue
            pad_line = (
                f"/*0x{pad_off:04x}*/ uint8_t                  "
                f"pad_0x{pad_off:04X}[0x{pad_size:x}]; // MQ-RE hotfix"
                + eol.decode("ascii")
            )
            # Determine indent from src[i] first-line indent
            # Use minimal leading chars that match the style of sibling decls
            # (upstream uses no leading space on these lines based on grep)
            inserts.append((next_idx, pad_line))
            log_entries.append(
                f"- PAD at 0x{pad_off:04X} size 0x{pad_size:x} after `{body}` (offset 0x{off:04X}, sz=0x{sz:X}, gap=0x{gap:X})"
            )

    # Apply inserts in reverse order
    out = list(src)
    for idx, padline in sorted(inserts, key=lambda t: -t[0]):
        out.insert(idx, padline)

    DST.write_bytes("".join(out).encode("utf-8"))
    LOG.write_text("# PZC hotfix pad log\n\n" + "\n".join(log_entries) + "\n")
    print(f"Wrote {DST} with {len(inserts)} pad inserts", file=sys.stderr)
    print(f"Log at {LOG}", file=sys.stderr)
    for e in log_entries[:20]:
        print(e, file=sys.stderr)


if __name__ == "__main__":
    main()
