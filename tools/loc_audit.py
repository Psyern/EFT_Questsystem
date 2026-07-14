#!/usr/bin/env python3
"""Localization audit for the DME Quest Framework.

Checks that every #STR_DME_ key referenced in source is defined in
DME_Tasks_Core/stringtable.csv, and that the table itself is well-formed.

Exit code 0 = clean, 1 = findings. Run from the mod root:
    python tools/loc_audit.py
"""

import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STRINGTABLE = ROOT / "DME_Tasks_Core" / "stringtable.csv"

EXPECTED_HEADER = [
    "Language", "original", "english", "czech", "german", "russian",
    "polish", "hungarian", "italian", "spanish", "french", "chinese",
    "japanese", "portuguese", "chinesesimp",
]

# Script/layout/json references carry '#'; inputs.xml loc= carries none.
# Keys never end in '_' (keeps doc references like "#STR_DME_TASKS_LOCK_*" out).
KEY_WITH_HASH = re.compile(r"#(STR_DME_TASKS_[A-Z0-9_]*[A-Z0-9])\b")
KEY_IN_INPUTS = re.compile(r'loc="(STR_DME_TASKS_[A-Z0-9_]+)"')

SOURCE_GLOBS = ["**/*.c", "**/*.layout", "**/*.xml", "**/*.json"]
SKIP_DIRS = {".git", "tools", "loc_work", "docs", "data"}

# Keys defined for future surfaces (e.g. origin labels not yet rendered).
# Listed here so the orphan check stays honest: additions must be deliberate.
ALLOWED_ORPHANS: set[str] = set()

# Enum display keys are BUILT AT RUNTIME by DME_Tasks_EnumUtil concatenation
# ("#STR_DME_TASKS_STATE_" + name) — grep cannot see them. Verified instead
# against the enum value names in check_enum_keys().
RUNTIME_PREFIXES = (
    "STR_DME_TASKS_STATE_", "STR_DME_TASKS_CAT_", "STR_DME_TASKS_FACTION_",
    "STR_DME_TASKS_OBJ_", "STR_DME_TASKS_ORIGIN_",
)

ENUM_FILES = {
    "STR_DME_TASKS_STATE_": "EDME_Tasks_QuestState.c",
    "STR_DME_TASKS_CAT_": "EDME_Tasks_QuestCategory.c",
    "STR_DME_TASKS_FACTION_": "EDME_Tasks_Faction.c",
    "STR_DME_TASKS_OBJ_": "EDME_Tasks_ObjectiveType.c",
    "STR_DME_TASKS_ORIGIN_": "EDME_Tasks_OriginType.c",
}
ENUM_DIR = ROOT / "DME_Tasks_Core" / "scripts" / "3_Game" / "DME_Tasks_Core" / "Enums"
ENUM_EXTRA = {"STR_DME_TASKS_STATE_UNKNOWN"}  # fallback key, not an enum value


def iter_source_files():
    for pattern in SOURCE_GLOBS:
        for path in ROOT.glob(pattern):
            # Skip only ROOT-LEVEL dirs (tools/, docs/, data/ images) — nested
            # dirs like DME_Tasks_UI/data/ (inputs.xml!) must be scanned.
            rel = path.relative_to(ROOT)
            if rel.parts and rel.parts[0] in SKIP_DIRS:
                continue
            if path == STRINGTABLE:
                continue
            # LocKeys.c DEFINES constants; counting it as a reference would
            # make every constant self-referencing and kill orphan detection.
            if path.name == "DME_Tasks_LocKeys.c":
                continue
            yield path


def collect_constant_usage():
    """Constants declared in LocKeys.c -> keys, counted as referenced only
    when some OTHER file actually uses DME_Tasks_LocKeys.<NAME>."""
    lockeys = ROOT / "DME_Tasks_Core" / "scripts" / "3_Game" / "DME_Tasks_Core" / "Util" / "DME_Tasks_LocKeys.c"
    decl = re.compile(r'static const string (\w+) = "#(STR_DME_TASKS_[A-Z0-9_]+)"')
    const_to_key = {}
    if lockeys.exists():
        for match in decl.finditer(lockeys.read_text(encoding="utf-8")):
            const_to_key[match.group(1)] = match.group(2)

    used = {}  # key -> [(file, line)]
    pattern = re.compile(r"DME_Tasks_LocKeys\.(\w+)")
    for path in iter_source_files():
        if path.suffix != ".c":
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        for line_no, line in enumerate(text.splitlines(), 1):
            comment_at = line.find("//")
            if comment_at != -1:
                line = line[:comment_at]
            for match in pattern.finditer(line):
                key = const_to_key.get(match.group(1))
                if key:
                    used.setdefault(key, []).append((path, line_no))
    return const_to_key, used


def check_enum_keys(table):
    """Runtime-built enum keys: every enum value needs its key, and every
    enum-area key must match a real enum value (or ENUM_EXTRA)."""
    problems = []
    value_re = re.compile(r"^\s*([A-Z][A-Z0-9_]*)\s*[=,}]", re.MULTILINE)
    for prefix, filename in ENUM_FILES.items():
        enum_path = ENUM_DIR / filename
        if not enum_path.exists():
            problems.append(f"enum file missing: {enum_path}")
            continue
        text = enum_path.read_text(encoding="utf-8", errors="replace")
        values = set(value_re.findall(text))
        defined = {k for k in table if k.startswith(prefix)}
        for value in values:
            if prefix + value not in table:
                problems.append(f"enum {filename}: value {value} has no key {prefix}{value}")
        for key in defined:
            suffix = key[len(prefix):]
            if suffix not in values and key not in ENUM_EXTRA:
                problems.append(f"{key} matches no value in {filename}")
    return problems


def collect_referenced_keys():
    refs = {}  # key -> [(file, line_no)]
    for path in iter_source_files():
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        is_script = path.suffix == ".c"
        for line_no, line in enumerate(text.splitlines(), 1):
            scan = line
            if is_script:
                comment_at = scan.find("//")
                if comment_at != -1:
                    scan = scan[:comment_at]
            for match in KEY_WITH_HASH.finditer(scan):
                refs.setdefault(match.group(1), []).append((path, line_no))
            for match in KEY_IN_INPUTS.finditer(scan):
                refs.setdefault(match.group(1), []).append((path, line_no))
    return refs


def load_stringtable():
    problems = []
    keys = {}  # key -> row dict
    if not STRINGTABLE.exists():
        problems.append(f"FATAL: {STRINGTABLE} does not exist")
        return keys, problems

    with STRINGTABLE.open(encoding="utf-8-sig", newline="") as handle:
        reader = csv.reader(handle)
        rows = list(reader)

    if not rows:
        problems.append("FATAL: stringtable.csv is empty")
        return keys, problems

    # The Expansion-style header ends with a trailing comma -> one empty trailing field.
    header = [cell for cell in rows[0] if cell != ""]
    if header != EXPECTED_HEADER:
        problems.append(
            "Header mismatch:\n  expected " + ",".join(EXPECTED_HEADER)
            + "\n  found    " + ",".join(header)
        )

    for idx, row in enumerate(rows[1:], 2):
        cells = row[:-1] if row and row[-1] == "" else list(row)
        if not cells:
            continue
        key = cells[0]
        if not key.startswith("STR_DME_TASKS_"):
            problems.append(f"line {idx}: key '{key}' does not match STR_DME_TASKS_*")
        if len(cells) != len(EXPECTED_HEADER):
            problems.append(
                f"line {idx} ({key}): {len(cells)} fields, expected {len(EXPECTED_HEADER)}"
            )
            continue
        empties = [EXPECTED_HEADER[i] for i, cell in enumerate(cells) if cell == ""]
        if empties:
            problems.append(
                f"line {idx} ({key}): empty cells in {', '.join(empties)} — fill with the English text"
            )
        if key in keys:
            problems.append(f"line {idx}: duplicate key {key}")
        keys[key] = dict(zip(EXPECTED_HEADER, cells))
    return keys, problems


def check_params(refs, table):
    """Every key whose English text contains %1 must be resolvable with params.

    Static heuristic: flag %1-keys that are referenced from a context with no
    obvious param plumbing (Resolve with a single argument on the same line).
    """
    problems = []
    for key, row in table.items():
        if "%1" not in row["original"]:
            continue
        for path, line_no in refs.get(key, []):
            line = path.read_text(encoding="utf-8", errors="replace").splitlines()[line_no - 1]
            if "Resolve(" in line and f'"#{key}"' in line.replace(" ", ""):
                # Resolve("#KEY") with no further args on the same line
                collapsed = line.replace(" ", "")
                if f'Resolve("#{key}")' in collapsed:
                    problems.append(
                        f"{path.relative_to(ROOT)}:{line_no}: {key} takes %1 but Resolve() gets no params"
                    )
    return problems


def main():
    refs = collect_referenced_keys()
    table, problems = load_stringtable()

    const_to_key, const_used = collect_constant_usage()
    for key, spots in const_used.items():
        refs.setdefault(key, []).extend(spots)

    missing = sorted(set(refs) - set(table))
    orphans = sorted(
        key for key in set(table) - set(refs) - ALLOWED_ORPHANS
        if not key.startswith(RUNTIME_PREFIXES)
    )
    problems.extend(check_enum_keys(table))

    for key in missing:
        spots = ", ".join(f"{p.relative_to(ROOT)}:{n}" for p, n in refs[key][:3])
        problems.append(f"MISSING key {key} (referenced at {spots}) — would render as raw #STR_ in game")
    for key in orphans:
        problems.append(f"ORPHAN key {key} — defined in stringtable.csv but never referenced")

    problems.extend(check_params(refs, table))

    print(f"referenced keys : {len(refs)}")
    print(f"defined keys    : {len(table)}")
    print(f"missing         : {len(missing)}")
    print(f"orphans         : {len(orphans)}")

    if problems:
        print("\n--- findings ---")
        for problem in problems:
            print(problem)
        return 1

    print("\nOK — stringtable coverage is complete.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
