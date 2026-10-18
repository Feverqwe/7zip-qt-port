#!/usr/bin/env python3
"""Capture existing UI scenarios with the host's native Qt color schemes."""
import argparse
import html
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parent.parent
CATALOG = ROOT / "scripts/config/ui-review.json"


def gallery(output, runs):
    cards = []
    for run in runs:
        for shot in run["captures"]:
            title = f'{run["title"]} · {run["scheme"]} · {shot["tag"]}'
            esc = html.escape
            image = esc(shot["image"], quote=True)
            report = esc(shot["report"], quote=True)
            cards.append(
                f'<article data-group="{esc(run["id"], quote=True)}" '
                f'data-scheme="{run["scheme"]}" data-search="{esc(title.lower(), quote=True)}">'
                f'<a href="{image}"><img loading="lazy" src="{image}" alt="{esc(title)}"></a>'
                f'<p>{esc(title)}</p><small>{esc(shot["qpa"])} · '
                f'{shot["size"][0]}×{shot["size"][1]} · '
                f'<a href="{report}">widget JSON</a></small></article>'
            )
    problems = [f'{r["id"]} / {r["scheme"]}: {r["error"]}' for r in runs if r.get("error")]
    status = "Все выбранные сценарии сняты." if not problems else "Есть незавершённые сценарии."
    document = """<!doctype html><html lang="ru"><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Sevenzip — ревью тем</title><style>
:root {color-scheme:light dark;font-family:system-ui,sans-serif}
body {margin:2rem;line-height:1.5} header {max-width:75rem} h1 {margin-bottom:.3rem}
input {font:inherit;padding:.6rem;width:min(36rem,90%);margin:1rem 0}
main {display:grid;grid-template-columns:repeat(auto-fill,minmax(320px,1fr));gap:1.5rem}
article {border:1px solid GrayText;border-radius:8px;padding:1rem;min-width:0}
img {width:100%;height:280px;object-fit:contain;object-position:top;background:Canvas}
p {overflow-wrap:anywhere;margin:.6rem 0}small {color:GrayText}[hidden]{display:none}
</style><header><h1>Sevenzip · ревью адаптивных цветов</h1>
<p>Нативные Qt-окна на локальных тестовых данных. Нажмите снимок для полного размера.
Системные настройки ОС не менялись. Снимки показывают клиентскую область окон;
нативные панели выбора ОС и заголовки окон сюда не входят.</p>
<p>STATUS</p><details><summary>Сведения о сборе</summary><pre>DETAILS</pre></details>
<select id="group" aria-label="Группа окон"><option value="">Все окна</option>GROUPS</select>
<select id="scheme" aria-label="Тема"><option value="">Обе темы</option><option>light</option><option>dark</option></select>
<input type="search" placeholder="Поиск: окно, en, ru…" aria-label="Поиск снимков">
<p id="count"></p></header><main>CARDS</main><script>
const input=document.querySelector('input'), group=document.querySelector('#group'), scheme=document.querySelector('#scheme'), cards=[...document.querySelectorAll('article')];
function filter(){let n=0;const q=input.value.toLowerCase();for(const c of cards){c.hidden=!c.dataset.search.includes(q)||(group.value&&c.dataset.group!==group.value)||(scheme.value&&c.dataset.scheme!==scheme.value);if(!c.hidden)n++;}document.querySelector('#count').textContent=`${n} / ${cards.length} снимков`;}
input.addEventListener('input',filter);group.addEventListener('change',filter);scheme.addEventListener('change',filter);filter();</script></html>"""
    document = document.replace("STATUS", html.escape(status))
    document = document.replace("DETAILS", html.escape("\n".join(problems) or "См. manifest.json и журналы сценариев."))
    document = document.replace("CARDS", "\n".join(cards))
    groups = dict.fromkeys((run["id"], run["title"]) for run in runs)
    document = document.replace("GROUPS", "".join(
        f'<option value="{html.escape(key, quote=True)}">{html.escape(title)}</option>'
        for key, title in groups))
    (output / "index.html").write_text(document, encoding="utf-8")


def collect(directory, output, scheme, qpa):
    captures = []
    errors = []
    for path in sorted(directory.glob("*.json")):
        image = path.with_suffix(".png")
        if not image.is_file():
            continue
        data = json.loads(path.read_text(encoding="utf-8"))
        metadata = data["capture"]
        if metadata.get("colorScheme") != scheme:
            errors.append(f'{path.name}: requested {scheme}, got {metadata.get("colorScheme")}')
        if metadata["qpa"] != qpa:
            errors.append(f'{path.name}: requested native {qpa}, got {metadata["qpa"]}')
        captures.append({"tag": path.stem, "image": image.relative_to(output).as_posix(),
                         "report": path.relative_to(output).as_posix(),
                         "size": data["bounds"][2:], "qpa": metadata["qpa"],
                         "effectiveScheme": metadata.get("colorScheme"),
                         "palette": metadata.get("palette"), "window": data.get("name"),
                         "class": data["class"]})
    if not captures:
        errors.append("No screenshots produced")
    return captures, errors


def main():
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--only", nargs="+", choices=[row["id"] for row in catalog])
    parser.add_argument("--schemes", nargs="+", choices=["light", "dark"], default=["light", "dark"])
    parser.add_argument("--output", type=Path)
    parser.add_argument("--list", action="store_true")
    args = parser.parse_args()
    selected = [row for row in catalog if not args.only or row["id"] in args.only]
    if args.list:
        for row in selected:
            print(f'{row["id"]}: {row["title"]} ({row["target"]})')
        return 0
    system = platform.system()
    qpa = {"Darwin": "cocoa", "Windows": "windows", "Linux": "xcb"}[system]
    output = (args.output or ROOT / "build/dev/ui-theme-review").resolve()
    output.mkdir(parents=True, exist_ok=True)
    targets = list(dict.fromkeys(row["target"] for row in selected))
    subprocess.run([sys.executable, "scripts/prepare-runtime.py"], cwd=ROOT, check=True)
    subprocess.run(["bash", "scripts/build.sh", "dev", *targets], cwd=ROOT, check=True)
    previous = output / "manifest.json"
    runs = json.loads(previous.read_text(encoding="utf-8")).get("runs", []) if previous.exists() else []
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    source_diff = subprocess.check_output(["git", "diff", "HEAD", "--", "src", "CMakeLists.txt", "cmake"], cwd=ROOT)
    source_state = {"modified": bool(source_diff), "diffSha256": hashlib.sha256(source_diff).hexdigest()}
    host = {"Darwin": "macos", "Windows": "windows", "Linux": "linux"}[system]
    failures = 0
    for row in selected:
        for scheme in args.schemes:
            directory = output / host / scheme / row["id"]
            directory.mkdir(parents=True, exist_ok=True)
            # Only replace prior artifacts recorded for this exact producer.
            old = [run for run in runs if (run["id"], run["scheme"], run["host"]) ==
                   (row["id"], scheme, host)]
            for run in old:
                for shot in run["captures"]:
                    for key in ("image", "report"):
                        artifact = (output / shot[key]).resolve()
                        if artifact.parent == directory:
                            artifact.unlink(missing_ok=True)
            settings = directory / "settings.ini"
            env = os.environ | {"QT_QPA_PLATFORM": qpa, "SEVENZIP_REVIEW_COLOR_SCHEME": scheme,
                                "SEVENZIP_CAPTURE_SCREENSHOTS": "1",
                                "SEVENZIP_UI_REPORT_DIR": str(directory),
                                "SEVENZIP_SETTINGS_PATH": str(settings)}
            runtime = os.environ.get("SEVENZIP_CODEC_MODULE_DIR", str(ROOT / "build/codec-runtime"))
            env["PATH"] = runtime + os.pathsep + env.get("PATH", "")
            executable = ROOT / "build/dev" / row["target"]
            if system == "Windows":
                executable = executable.with_suffix(".exe")
            print(f'Capture {host}/{scheme}/{row["id"]}', flush=True)
            error = []
            with (directory / "run.log").open("w", encoding="utf-8") as log:
                try:
                    result = subprocess.run([str(executable), *row["functions"], "-silent"],
                                            cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT,
                                            timeout=180)
                    if result.returncode:
                        error.append(f'Producer exited {result.returncode}; see run.log')
                except subprocess.TimeoutExpired:
                    error.append("Producer timed out; see run.log")
            captures, issues = collect(directory, output, scheme, qpa)
            error.extend(issues)
            settings.unlink(missing_ok=True)
            run = {**row, "scheme": scheme, "host": host, "revision": revision, "sourceState": source_state,
                   "capturedAt": datetime.now(timezone.utc).isoformat(), "captures": captures}
            if error:
                failures += 1
                run["error"] = "; ".join(error)
                print(run["error"], flush=True)
            runs = [r for r in runs if r not in old] + [run]
            previous.write_text(json.dumps({"runs": runs}, ensure_ascii=False, indent=2), encoding="utf-8")
            gallery(output, runs)
    print(f'Review: {output / "index.html"}; failed scenarios: {failures}', flush=True)
    return int(bool(failures))


if __name__ == "__main__":
    sys.exit(main())
