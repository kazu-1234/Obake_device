# -*- coding: utf-8 -*-
"""repos.json の git リポを取る。SSID などは扱わない。"""
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def main() -> None:
    repos = json.loads((ROOT / "repos.json").read_text(encoding="utf-8"))
    for item in repos:
        dest = ROOT / item["path"]
        url = item["url"]
        branch = item.get("branch", "main")
        if dest.exists():
            print("skip exists:", dest)
            continue
        dest.parent.mkdir(parents=True, exist_ok=True)
        cmd = ["git", "clone", "--branch", branch, "--depth", "1", url, str(dest)]
        print(" ".join(cmd))
        subprocess.check_call(cmd)


if __name__ == "__main__":
    main()
