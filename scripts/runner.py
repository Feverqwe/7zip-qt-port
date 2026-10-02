#!/usr/bin/env python3
"""Compatibility entrypoint for the globally installed runners skill."""
import os
from pathlib import Path
import runpy
import sys

project = Path(__file__).resolve().parent.parent
entry = Path(os.environ.get('CODEX_HOME', str(Path.home() / '.codex'))) / 'skills/runners/scripts/runner.py'
if not entry.is_file():
    raise SystemExit(f'Install the global runners skill first: {entry}')
if not any(argument == '--project' or argument.startswith('--project=') for argument in sys.argv[1:]):
    sys.argv[1:1] = ['--project', str(project)]
sys.path.insert(0, str(entry.parent))
runpy.run_path(str(entry), run_name='__main__')
