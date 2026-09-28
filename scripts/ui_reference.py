"""Read desktop 7zFM dialog resources without a Windows SDK.

Local code, not borrowed 7-Zip logic. A real C preprocessor handles macros;
SDK headers are deliberately opaque and resource IDs stay symbolic.
"""
import ast
import hashlib
import re
import shutil
import subprocess
import tempfile
from pathlib import Path


KINDS = {
    "LTEXT": "QLabel", "RTEXT": "QLabel", "CTEXT": "QLabel",
    "PUSHBUTTON": "QPushButton", "DEFPUSHBUTTON": "QPushButton",
    "GROUPBOX": "QGroupBox", "EDITTEXT": "QLineEdit",
    "COMBOBOX": "QComboBox", "CHECKBOX": "QCheckBox",
    "AUTOCHECKBOX": "QCheckBox", "AUTORADIOBUTTON": "QRadioButton",
    "LISTBOX": "QListWidget", "CONTROL": None,
}
TOKEN = re.compile(r'L?"(?:[^"\\]|\\.|"")*"|[A-Za-z_]\w*|0[xX][0-9a-fA-F]+|\d+|[^\s]')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def integer(tokens):
    """Evaluate arithmetic only, never execute resource expressions."""
    tree = ast.parse(" ".join(tokens), mode="eval")

    def visit(node):
        if isinstance(node, ast.Constant) and type(node.value) is int:
            return node.value
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.UAdd, ast.USub)):
            return visit(node.operand) * (-1 if isinstance(node.op, ast.USub) else 1)
        if isinstance(node, ast.BinOp):
            a, b = visit(node.left), visit(node.right)
            if isinstance(node.op, ast.Add):
                return a + b
            if isinstance(node.op, ast.Sub):
                return a - b
            if isinstance(node.op, ast.Mult):
                return a * b
        raise ValueError("Unsupported coordinate expression: " + " ".join(tokens))

    return visit(tree.body)


def fields(tokens):
    result, field = [], []
    for token in tokens:
        if token == ",":
            result.append(field)
            field = []
        else:
            field.append(token)
    return result + [field]


def string(tokens):
    return ast.literal_eval(" ".join(tokens).removeprefix("L"))


def preprocess(root, resource, compiler):
    root = Path(root).resolve()
    resource = str((root / resource).resolve().relative_to(root))
    dependencies, opaque = {}, set()
    with tempfile.TemporaryDirectory(prefix="sevenzip-ui-") as directory:
        stage = Path(directory)

        def copy(path):
            path = path.resolve()
            relative = path.relative_to(root)
            if str(relative) in dependencies:
                return
            dependencies[str(relative)] = digest(path)
            text = path.read_text(encoding="utf-8-sig")

            def include(match):
                quote, name = match.groups()
                if quote == '<':
                    if name.lower() not in {"windows.h", "commctrl.h", "winnt.h", "winuser.h"}:
                        raise ValueError(f"Unsupported SDK include: {name}")
                    opaque.add(name)
                    return "// SDK header omitted: " + name
                copy(path.parent / name)
                return match.group(0)

            text = re.sub(r'^\s*#\s*include\s*([<"])([^>"\n]+)[>"]', include, text,
                          flags=re.MULTILINE)
            # Keep identity across #included *Res.h files, including IDOK etc.
            text = re.sub(r'^(\s*#\s*define\s+)(ID\w*)\s+[^\n]+',
                          lambda m: m[1] + m[2] + " " + m[2], text, flags=re.MULTILINE)
            target = stage / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(text, encoding="utf-8")

        copy(root / resource)
        command = [compiler, "-E", "-P", "-x", "c", "-undef", "-nostdinc",
                   "-DRC_INVOKED", str(stage / resource)]
        result = subprocess.run(command, capture_output=True, text=True, check=True)
        return result.stdout, dependencies, sorted(opaque)


def parse_dialogs(text):
    tokens = TOKEN.findall(text)
    dialogs = []
    i = 0
    while i < len(tokens):
        if tokens[i] not in {"DIALOG", "DIALOGEX"}:
            i += 1
            continue
        identity = tokens[i - 1]
        start = i + 1
        while tokens[i] not in {"BEGIN", "{"}:
            i += 1
            if i >= len(tokens):
                raise ValueError("Unterminated dialog " + identity)
        header = tokens[start:i]
        end_rect = next((n for n, t in enumerate(header) if t in {"STYLE", "FONT", "CAPTION"}),
                        len(header))
        rect = [integer(f) for f in fields(header[:end_rect])[:4]]
        caption = string([header[header.index("CAPTION") + 1]]) if "CAPTION" in header else ""
        controls = []
        i += 1
        while i < len(tokens) and tokens[i] not in {"END", "}"}:
            kind = tokens[i]
            if kind not in KINDS:
                raise ValueError(f"Unsupported resource statement in {identity}: {kind}")
            start = i + 1
            i = start
            while i < len(tokens) and tokens[i] not in {*KINDS, "END", "}"}:
                i += 1
            parts = fields(tokens[start:i])
            has_text = kind not in {"COMBOBOX", "EDITTEXT", "LISTBOX"}
            offset = 1 if has_text else 0
            control_id = " ".join(parts[offset])
            coord = offset + (3 if kind == "CONTROL" else 1)
            bounds = [integer(f) for f in parts[coord:coord + 4]]
            if len(bounds) != 4:
                raise ValueError("Missing bounds for " + control_id)
            style = " ".join(parts[2 + offset]) if kind == "CONTROL" else " ".join(
                t for p in parts[coord + 4:] for t in p)
            expected = KINDS[kind]
            if kind == "CONTROL":
                expected = ("QCheckBox" if "BS_AUTOCHECKBOX" in style else
                            "QRadioButton" if "BS_AUTORADIOBUTTON" in style else None)
            control = {"id": control_id, "kind": kind, "qtClass": expected,
                       "text": string(parts[0]) if has_text else None,
                       "boundsDLU": bounds, "style": style}
            if kind == "COMBOBOX":
                control["editable"] = "CBS_DROPDOWNLIST" not in style and "CBS_DROPDOWN" in style
            controls.append(control)
        if i == len(tokens):
            raise ValueError("Unterminated dialog " + identity)
        dialogs.append({"id": identity, "caption": caption, "boundsDLU": rect,
                        "controls": controls})
        i += 1
    if not dialogs:
        raise ValueError("No supported dialog resources found")
    return dialogs


def extract(root, resource, behavior_sources=(), compiler=None):
    root = Path(root).resolve()
    compiler = compiler or shutil.which("clang") or shutil.which("gcc")
    if not compiler:
        raise ValueError("clang or gcc is required for resource preprocessing")
    revision = subprocess.run(["git", "-C", str(root), "rev-parse", "HEAD"],
                              capture_output=True, text=True, check=True).stdout.strip()
    text, dependencies, opaque = preprocess(root, resource, compiler)
    candidates = []
    symbols = {}
    for source in behavior_sources:
        source = str((root / source).resolve().relative_to(root))
        path = root / source
        dependencies[source] = digest(path)
        symbols[source] = sorted(set(re.findall(r'^\s*(?:bool|void|INT_PTR)\s+([\w]+::On\w+)\s*\(',
                                                path.read_text(encoding="utf-8-sig"), re.MULTILINE)))
        for number, line in enumerate(path.read_text(encoding="utf-8-sig").splitlines(), 1):
            if re.search(r'::On\w+\s*\(|\b(?:Show\w*|Enable\w*|CheckButton|SetItemText|'
                         r'MoveItem|ChangeSubWindow\w*|SetText|SetCurSel)\s*\(', line):
                candidates.append({"source": source, "line": number, "code": line.strip()})
    for source, expected in dependencies.items():
        original = subprocess.run(["git", "-C", str(root), "show", f"{revision}:{source}"],
                                  capture_output=True, check=True).stdout
        if hashlib.sha256(original).hexdigest() != expected:
            raise ValueError("Checkout differs from recorded upstream commit: " + source)
    return {"schema": 1, "upstreamCommit": revision, "resource": resource,
            "profile": "desktop; RC_INVOKED; UNDER_CE undefined; SDK constants symbolic",
            "dependencies": dependencies, "opaqueIncludes": opaque,
            "dialogs": parse_dialogs(text), "behaviorSymbols": symbols,
            "behaviorCandidates": candidates,
            "limitations": ["DLU are not pixels; combo height includes dropdown extent",
                            "Behavior candidates are navigation hints, not a C++ semantic analysis",
                            "No Windows rendering, focus or tab-order verification"]}
