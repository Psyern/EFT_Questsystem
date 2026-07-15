#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Baut die self-contained docs/quest-editor/index.html aus den modularen src/-Dateien.

Aufruf:  python build.py
Ergebnis: index.html (ein <style> aus styles.css, Body aus shell.html, EIN <script> aus den
JS-Modulen in Abhaengigkeitsreihenfolge). Keine externen Ressourcen — bleibt offline-/CSP-fest.
"""
import os

BASE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(BASE, "src")

# Abhaengigkeitsreihenfolge (state.js definiert $, canvas, ... vor render/interact; main.js ruft init() zuletzt)
JS_ORDER = [
    "constants.js", "model.js", "state.js", "render.js", "interact.js",
    "inspector.js", "validate.js", "io.js", "main.js",
]


def read(name):
    with open(os.path.join(SRC, name), encoding="utf-8") as f:
        return f.read().rstrip("\n")


def main():
    styles = read("styles.css")
    shell = read("shell.html")
    js = "\n\n".join(read(n) for n in JS_ORDER)

    html = (
        "<!DOCTYPE html>\n"
        '<html lang="de">\n'
        "<head>\n"
        '<meta charset="UTF-8">\n'
        '<meta name="viewport" content="width=device-width, initial-scale=1.0">\n'
        "<title>DME Quest Editor</title>\n"
        "<style>\n" + styles + "\n</style>\n"
        "</head>\n"
        "<body>\n" + shell + "\n\n"
        "<script>\n"
        '"use strict";\n' + js + "\n"
        "</script>\n"
        "</body>\n"
        "</html>\n"
    )

    out = os.path.join(BASE, "index.html")
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write(html)
    print("built index.html: %d chars (%d JS modules)" % (len(html), len(JS_ORDER)))


if __name__ == "__main__":
    main()
