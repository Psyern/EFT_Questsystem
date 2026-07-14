#!/usr/bin/env python3
"""Generate DME_Tasks_LocKeys.c and DME_Tasks_Core/stringtable.csv from the
localization registry (loc_work/LOC_INVENTORY.md + loc_work/LOC_ENUMS.md
+ optional loc_work/SEED_KEYS.md).

Single source of truth: the registry markdown tables. Re-run after editing them.
    python tools/loc_gen.py
"""

import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LOC_WORK = ROOT.parent / "loc_work"
STRINGTABLE = ROOT / "DME_Tasks_Core" / "stringtable.csv"
LOCKEYS = ROOT / "DME_Tasks_Core" / "scripts" / "3_Game" / "DME_Tasks_Core" / "Util" / "DME_Tasks_LocKeys.c"

HEADER = [
    "Language", "original", "english", "czech", "german", "russian",
    "polish", "hungarian", "italian", "spanish", "french", "chinese",
    "japanese", "portuguese", "chinesesimp",
]

# Renames decided at the Wave-0 merge barrier: enum-area keys are mechanical
# (match the enum value name) so EnumUtil display helpers can concatenate.
RENAMES = {
    "STR_DME_TASKS_STATE_READY": "STR_DME_TASKS_STATE_READY_TO_TURN_IN",
    "STR_DME_TASKS_MENU_CAT_STORY": "STR_DME_TASKS_CAT_STORY",
    "STR_DME_TASKS_MENU_CAT_SIDE": "STR_DME_TASKS_CAT_SIDE",
    "STR_DME_TASKS_MENU_CAT_FACTION": "STR_DME_TASKS_CAT_FACTION",
    "STR_DME_TASKS_MENU_CAT_BOSS": "STR_DME_TASKS_CAT_BOSS",
    "STR_DME_TASKS_MENU_CAT_EXPEDITION": "STR_DME_TASKS_CAT_EXPEDITION",
    "STR_DME_TASKS_MENU_CAT_DAILY": "STR_DME_TASKS_CAT_DAILY",
    "STR_DME_TASKS_MENU_CAT_WEEKLY": "STR_DME_TASKS_CAT_WEEKLY",
}

# Constants are emitted for every key except these areas:
#  - enum areas (STATE/CAT/FACTION/OBJ/ORIGIN): built by DME_Tasks_EnumUtil helpers
#  - INPUT: referenced from inputs.xml only (no '#', no script access)
NO_CONSTANT_PREFIXES = (
    "STR_DME_TASKS_STATE_", "STR_DME_TASKS_CAT_", "STR_DME_TASKS_FACTION_",
    "STR_DME_TASKS_OBJ_", "STR_DME_TASKS_ORIGIN_", "STR_DME_TASKS_INPUT_",
)


def unescape(cell):
    cell = cell.strip()
    verbatim = False
    if cell.startswith("`") and cell.endswith("`") and len(cell) >= 2:
        # Backtick-wrapped cells are verbatim — leading/trailing spaces inside
        # are significant (concatenated suffix strings like " | REPEATABLE").
        cell = cell[1:-1]
        verbatim = True
    cell = cell.replace("\\|", "|").replace("\\<", "<").replace("\\>", ">")
    if verbatim:
        return cell
    return cell.strip()


def parse_tables(path, key_col, en_col, de_col):
    """Yield (key, en, de) from every markdown table in the file."""
    out = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.startswith("|"):
            continue
        # Split on unescaped pipes only — cells may contain literal '\|'.
        raw_cells = re.split(r"(?<!\\)\|", line.strip().strip("|"))
        cells = [unescape(c) for c in raw_cells]
        if len(cells) <= max(key_col, en_col, de_col):
            continue
        key = cells[key_col]
        if not re.fullmatch(r"STR_DME_TASKS_[A-Z0-9_]+", key):
            continue
        en = cells[en_col]
        de = cells[de_col]
        if en == "" or en.lower() in ("en", "proposed en"):
            continue
        out.append((key, en, de))
    return out


def main():
    entries = {}  # key -> (en, de)

    def add(key, en, de, source):
        key = RENAMES.get(key, key)
        if key in entries and entries[key] != (en, de):
            print(f"WARN: {key} redefined by {source}: keeping first "
                  f"({entries[key][0]!r}), ignoring ({en!r})")
            return
        entries[key] = (en, de)

    # LOC_INVENTORY.md: | File | Line | German | Key | Params | EN | DE |
    for key, en, de in parse_tables(LOC_WORK / "LOC_INVENTORY.md", 3, 5, 6):
        add(key, en, de, "LOC_INVENTORY.md")
    # LOC_ENUMS.md: | Enum value | Int | Key | EN | DE | Rendered where |
    for key, en, de in parse_tables(LOC_WORK / "LOC_ENUMS.md", 2, 3, 4):
        add(key, en, de, "LOC_ENUMS.md")
    # Optional seed-content keys: | Key | EN | DE | (written by Wave 3 / A3.1)
    seed = LOC_WORK / "SEED_KEYS.md"
    if seed.exists():
        for key, en, de in parse_tables(seed, 0, 1, 2):
            add(key, en, de, "SEED_KEYS.md")

    if not entries:
        print("FATAL: no keys parsed")
        return 1

    # --- stringtable.csv: original/english = EN, german = DE, all other
    # columns = EN verbatim (never empty, never machine-translated).
    with STRINGTABLE.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle, quoting=csv.QUOTE_ALL, lineterminator="\n")
        handle.write(",".join(f'"{h}"' for h in HEADER) + ",\n")
        for key in sorted(entries):
            en, de = entries[key]
            row = [key, en, en, en, de, en, en, en, en, en, en, en, en, en, en]
            writer.writerow(row)
            handle.seek(handle.tell() - 1)  # replace \n to append trailing comma
            handle.write(",\n")

    # --- DME_Tasks_LocKeys.c
    lines = [
        "//! Generated by tools/loc_gen.py from loc_work/LOC_INVENTORY.md + LOC_ENUMS.md — do not edit by hand.",
        "//! Every constant follows the '#' rule: the value is a stringtable key resolved on the client",
        "//! (DME_Tasks_Loc.Resolve fills %1/%2). Enum display keys (STATE_/CAT_/FACTION_/OBJ_/ORIGIN_)",
        "//! have no constants — DME_Tasks_EnumUtil builds them mechanically from the enum value name.",
        "class DME_Tasks_LocKeys {",
    ]
    area = ""
    for key in sorted(entries):
        if key.startswith(NO_CONSTANT_PREFIXES):
            continue
        name = key.replace("STR_DME_TASKS_", "")
        this_area = name.split("_")[0]
        if this_area != area:
            area = this_area
            lines.append("")
            lines.append(f"\t// --- {area} ---")
        lines.append(f'\tstatic const string {name} = "#{key}";')
    lines.append("}")
    LOCKEYS.write_text("\n".join(lines) + "\n", encoding="utf-8")

    n_const = sum(1 for k in entries if not k.startswith(NO_CONSTANT_PREFIXES))
    print(f"stringtable.csv : {len(entries)} keys -> {STRINGTABLE}")
    print(f"LocKeys.c       : {n_const} constants -> {LOCKEYS}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
