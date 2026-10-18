"""Small stdlib-only lock protocol executed before syncing any guest source."""

import json
from pathlib import Path
import shutil

OWNER = "sevenzip-desktop/persistent-runner-v1"


def operate(root, token, action):
    root = Path(root)
    lock = root.with_name(root.name + ".lock")
    owner = root / ".runner-owner.json"
    complete = root / ".runner-complete"
    if root.is_symlink() or lock.is_symlink():
        raise ValueError("Refusing linked runner workspace/lock")
    if action == "acquire":
        root.parent.mkdir(parents=True, exist_ok=True)
        try:
            lock.mkdir()
        except FileExistsError:
            details = (lock / "owner.json").read_text() if (lock / "owner.json").is_file() else "initializing"
            raise ValueError(f"Runner worktree busy: {lock}; owner: {details}")
        try:
            (lock / "owner.json").write_text(json.dumps({"owner": OWNER, "token": token}))
            if root.exists() and any(root.iterdir()):
                if not owner.is_file() or owner.is_symlink() or json.loads(owner.read_text()).get("owner") != OWNER:
                    raise ValueError("Refusing to sync/delete an unowned existing worktree")
            root.mkdir(exist_ok=True)
            owner.write_text(json.dumps({"owner": OWNER}))
            complete.unlink(missing_ok=True)
        except BaseException:
            shutil.rmtree(lock)
            raise
    elif action in ("release", "release-completed"):
        record = json.loads((lock / "owner.json").read_text())
        if record != {"owner": OWNER, "token": token}:
            raise ValueError("Runner lock owner mismatch")
        if action == "release-completed" and (not complete.is_file() or complete.read_text().strip() != token):
            raise ValueError(f"Execution completion is unconfirmed; lock retained: {lock}")
        shutil.rmtree(lock)
    elif action == "status":
        print((lock / "owner.json").read_text() if lock.is_dir() else "unlocked")
    else:
        raise ValueError("Unknown runner lock action")
