#!/usr/bin/env python3
"""
Bruce Firmware Cloner
Clones the Bruce firmware repository into a user-specified directory.
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path

REPO_URL = "https://github.com/pr3y/Bruce.git"
REPO_NAME = "Bruce"


def run_command(cmd, cwd=None):
    """Run a shell command and stream output."""
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"[!] Command failed: {' '.join(cmd)}")
        print(e.stdout)
        return False
    except FileNotFoundError:
        print(f"[!] Command not found: {cmd[0]}. Is it installed?")
        return False


def check_git():
    """Ensure git is installed and available."""
    try:
        subprocess.run(
            ["git", "--version"],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def clone_repo(target_dir, branch=None, depth=None):
    """Clone the Bruce firmware repo into target_dir."""
    target_path = Path(target_dir).expanduser().resolve()
    target_path.mkdir(parents=True, exist_ok=True)

    dest = target_path / REPO_NAME

    if dest.exists() and any(dest.iterdir()):
        print(f"[!] Destination already exists and is not empty: {dest}")
        overwrite = input("    Overwrite? This will delete the existing folder. (y/N): ").strip().lower()
        if overwrite != "y":
            print("[*] Aborted.")
            return None
        import shutil
        shutil.rmtree(dest)

    cmd = ["git", "clone"]

    if branch:
        cmd += ["-b", branch]
    if depth:
        cmd += ["--depth", str(depth)]

    cmd += [REPO_URL, str(dest)]

    print(f"[*] Cloning Bruce firmware into: {dest}")
    if run_command(cmd):
        print(f"[✓] Successfully cloned into: {dest}")
        return dest
    else:
        print("[!] Clone failed.")
        return None


def install_deps(repo_path):
    """Optionally install Python dependencies for building Bruce."""
    print("\n[*] Installing Python dependencies (platformio, esptool, etc.)...")
    deps = ["platformio", "requests", "esptool", "intelhex"]
    if run_command([sys.executable, "-m", "pip", "install", *deps]):
        print("[✓] Dependencies installed.")
    else:
        print("[!] Dependency install failed. You may need to run it manually.")


def main():
    parser = argparse.ArgumentParser(
        description="Clone the Bruce firmware repository into a directory of your choice."
    )
    parser.add_argument(
        "-d", "--dir",
        default=".",
        help="Target directory to clone into (default: current directory).",
    )
    parser.add_argument(
        "-b", "--branch",
        default=None,
        help="Specific branch to clone (default: repo default branch).",
    )
    parser.add_argument(
        "--depth",
        type=int,
        default=None,
        help="Shallow clone with given depth (e.g., 1 for latest only).",
    )
    parser.add_argument(
        "--deps",
        action="store_true",
        help="Install Python build dependencies after cloning.",
    )
    parser.add_argument(
        "--zip",
        action="store_true",
        help="Download as ZIP instead of git clone (no git required).",
    )

    args = parser.parse_args()

    if args.zip:
        download_zip(Path(args.dir).expanduser().resolve())
        return

    if not check_git():
        print("[!] Git is not installed or not in PATH.")
        print("    Install git or re-run with --zip to download a ZIP archive instead.")
        sys.exit(1)

    repo_path = clone_repo(args.dir, branch=args.branch, depth=args.depth)

    if repo_path and args.deps:
        install_deps(repo_path)

    if repo_path:
        print(f"\n[*] Done. Navigate to: cd {repo_path}")


def download_zip(target_dir):
    """Fallback: download Bruce as a ZIP archive using urllib (no git needed)."""
    import urllib.request
    import zipfile
    import io

    target_path = Path(target_dir).expanduser().resolve()
    target_path.mkdir(parents=True, exist_ok=True)

    zip_url = f"{REPO_URL[:-4]}/archive/refs/heads/main.zip"
    print(f"[*] Downloading ZIP from: {zip_url}")

    try:
        with urllib.request.urlopen(zip_url) as resp:
            data = resp.read()
    except Exception as e:
        print(f"[!] Download failed: {e}")
        return

    try:
        with zipfile.ZipFile(io.BytesIO(data)) as zf:
            zf.extractall(target_path)
        print(f"[✓] Extracted into: {target_path}")
    except zipfile.BadZipFile:
        print("[!] Downloaded file is not a valid ZIP.")


if __name__ == "__main__":
    main()