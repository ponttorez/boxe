#!/usr/bin/env python3
"""Builds the web version of the demo and records the GIFs and screenshots of the README.

    python3 demo/tools/media.py web                  # export the demo to demo/build/web
    python3 demo/tools/media.py web --templates      # also rebuild the web export template first (needs Emscripten)
    python3 demo/tools/media.py media                # record every scenario into demo/build/pages/media
    python3 demo/tools/media.py media motion layered # record some scenarios
    python3 demo/tools/media.py editor               # take the screenshots of the editor
    python3 demo/tools/media.py publish              # copy the web build and the media to the pages branch
    python3 demo/tools/media.py all                  # web, media, editor and publish

The engine is bin/godot.linuxbsd.editor.x86_64 unless the GODOT environment variable names another one.
The recordings use Movie Maker mode, so they don't depend on the speed of the computer. ffmpeg makes the GIFs.
Publishing commits to pages in a separate worktree. It only pushes with --push.
"""

import argparse
import contextlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

DEMO = Path(__file__).resolve().parent.parent
ROOT = DEMO.parent
BUILD = DEMO / "build"
WEB = BUILD / "web"
PAGES = BUILD / "pages"  # A worktree of the pages branch, which GitHub Pages serves.
MEDIA = PAGES / "media"
GODOT = Path(os.environ.get("GODOT", ROOT / "bin" / "godot.linuxbsd.editor.x86_64"))
WEB_TEMPLATE = ROOT / "bin" / "godot.web.template_release.wasm32.nothreads.zip"
FPS = 30

# Window size, GIF width and the part of the window in the GIF (x, y, width, height) of each scenario.
# Scenarios without a GIF only save stills. See tools/capture/scenarios.gd.
CONTENT = (360, 0, 1560, 1080)
SCENARIOS = {
    "motion": {"size": (1920, 1080), "gif_width": 960, "fps": 20},  # Scaled to the window, see motion.gd.
    "overview": {"size": (1920, 1080), "gif": False},
    "transitions": {"size": (1920, 1080), "crop": CONTENT, "gif_width": 860},
    "layered": {"size": (1920, 1080), "crop": CONTENT, "gif_width": 860},
    "sounds": {"size": (1920, 1080), "gif": False},
    "interactable": {"size": (1920, 1080), "crop": CONTENT, "gif_width": 860},
    "hit_areas": {"size": (1920, 1080), "crop": CONTENT, "gif_width": 860},
    "composites": {"size": (1920, 1080), "crop": CONTENT, "gif_width": 860},
    "bbcode": {"size": (1920, 1080), "crop": CONTENT, "gif_width": 860},
    "localization": {"size": (1920, 1080), "crop": CONTENT, "gif_width": 860},
    "mobile": {"size": (1920, 1080), "gif": False},
    "phone": {"size": (608, 1320), "gif": False},
}


def run(command, **kwargs):
    print("$", " ".join(str(part) for part in command), flush=True)
    return subprocess.run([str(part) for part in command], check=True, **kwargs)


def godot(*args, **kwargs):
    if not GODOT.exists():
        sys.exit(f"Godot not found at {GODOT}. Build the editor or set GODOT.")
    return run([GODOT, *args], **kwargs)


def build_web_template():
    run(["scons", "platform=web", "target=template_release", "threads=no", f"-j{os.cpu_count()}"], cwd=ROOT)


def export_web(templates=False):
    if templates or not WEB_TEMPLATE.exists():
        build_web_template()
    if WEB.exists():
        shutil.rmtree(WEB)
    WEB.mkdir(parents=True)
    godot("--headless", "--path", DEMO, "--import")
    godot("--headless", "--path", DEMO, "--export-release", "Web", WEB / "index.html")


@contextlib.contextmanager
def recording_window(size):
    """Movie Maker records the window, so it must have the size of the viewport and stay windowed.
    The settings are written to override.cfg, and an override.cfg of the user is restored afterwards."""
    override = DEMO / "override.cfg"
    previous = override.read_bytes() if override.exists() else None
    override.write_text(
        "[display]\n"
        f"window/size/viewport_width={size[0]}\n"
        f"window/size/viewport_height={size[1]}\n"
        "window/size/mode=0\n"
        "window/size/resizable=false\n"
    )
    try:
        yield
    finally:
        if previous is None:
            override.unlink(missing_ok=True)
        else:
            override.write_bytes(previous)


def make_gif(frames, info, name, settings):
    first = info["start_frame"]
    count = info["end_frame"] - first
    fps = settings.get("fps", 25)
    filters = []
    crop = info.get("crop", settings.get("crop"))
    if crop:
        x, y, w, h = crop
        filters.append(f"crop={w}:{h}:{x}:{y}")
    filters.append(f"fps={fps}")
    # Smaller crops keep their size instead of being enlarged.
    filters.append(f"scale='min({settings.get('gif_width', 860)},iw)':-2:flags=lanczos")
    filters.append(
        "split[a][b];[a]palettegen=max_colors=192:stats_mode=diff[p];[b][p]paletteuse=dither=bayer:bayer_scale=4:diff_mode=rectangle"
    )
    run([
        "ffmpeg",
        "-v",
        "error",
        "-y",
        "-framerate",
        FPS,
        "-start_number",
        first,
        "-i",
        frames / "frame%08d.png",
        "-frames:v",
        count,
        "-vf",
        ",".join(filters),
        "-loop",
        "0",
        MEDIA / f"{name}.gif",
    ])


def record(name):
    settings = SCENARIOS[name]
    MEDIA.mkdir(parents=True, exist_ok=True)
    with recording_window(settings["size"]):
        with tempfile.TemporaryDirectory(prefix=f"boxe_{name}_") as directory:
            frames = Path(directory)
            args = ["--path", DEMO, "res://tools/capture/capture.tscn", "--fixed-fps", FPS]
            if settings.get("gif", True):
                args += ["--write-movie", frames / "frame.png"]
            args += ["--", f"--scenario={name}", f"--out={MEDIA}", f"--locale={settings.get('locale', 'en')}"]
            godot(*args, timeout=600)
            info = json.loads((MEDIA / f"{name}.json").read_text())
            (MEDIA / f"{name}.json").unlink()
            if settings.get("gif", True):
                make_gif(frames, info, name, settings)


def record_media(names):
    for name in names or SCENARIOS:
        if name not in SCENARIOS:
            sys.exit(f"Unknown scenario {name}. Known: {', '.join(SCENARIOS)}")
        record(name)


def take_editor_shots():
    """Runs the editor on a copy of the demo with the capture plugin, with its own settings so the user's stay untouched."""
    MEDIA.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="boxe_editor_") as directory:
        project = Path(directory) / "project"
        shutil.copytree(DEMO, project, ignore=shutil.ignore_patterns("build", "editor", "override.cfg"))
        plugin = project / "addons" / "boxe_capture"
        shutil.copytree(DEMO / "tools" / "capture" / "editor", plugin)
        settings = (project / "project.godot").read_text()
        settings += '\n[editor_plugins]\n\nenabled=PackedStringArray("res://addons/boxe_capture/plugin.cfg")\n'
        (project / "project.godot").write_text(settings)
        environment = dict(os.environ)
        environment["XDG_CONFIG_HOME"] = str(Path(directory) / "config")
        environment["XDG_DATA_HOME"] = str(Path(directory) / "data")
        environment["XDG_CACHE_HOME"] = str(Path(directory) / "cache")
        environment["BOXE_CAPTURE_OUT"] = str(MEDIA)
        # A fresh editor at 100% scale, with popups inside the window so they are captured.
        config = Path(directory) / "config" / "godot"
        config.mkdir(parents=True)
        (config / "editor_settings-4.7.tres").write_text(
            '[gd_resource type="EditorSettings" format=3]\n\n[resource]\n'
            "interface/editor/display_scale = 2\n"
            "interface/editor/single_window_mode = true\n"
            'interface/editor/editor_language = "en"\n'
        )
        godot("--headless", "--path", project, "--import", env=environment)
        godot("--path", project, "--editor", "--resolution", "1600x900", env=environment, timeout=300)


def ensure_pages():
    """Checks out the pages branch into PAGES, where the media are recorded."""
    if PAGES.exists():
        return
    branches = subprocess.run(["git", "branch", "--list", "pages"], cwd=ROOT, capture_output=True, text=True).stdout
    if branches.strip():
        run(["git", "worktree", "add", PAGES, "pages"], cwd=ROOT)
    else:
        run(["git", "worktree", "add", "--orphan", "-b", "pages", PAGES], cwd=ROOT)


def publish(push=False):
    """Copies the web build next to the media in the pages branch, and commits them."""
    if not (WEB / "index.html").exists() or not MEDIA.exists():
        sys.exit("Run the web and media commands first.")
    worktree = PAGES
    for item in worktree.iterdir():
        if item.name not in (".git", "media"):
            shutil.rmtree(item) if item.is_dir() else item.unlink()
    shutil.copytree(WEB, worktree, dirs_exist_ok=True)
    (worktree / ".nojekyll").touch()
    run(["git", "add", "--all"], cwd=worktree)
    if subprocess.run(["git", "diff", "--cached", "--quiet"], cwd=worktree).returncode:
        run(["git", "commit", "-m", "Update the web demo and the media"], cwd=worktree)
    if push:
        run(["git", "push", "origin", "pages"], cwd=worktree)
    else:
        print("Committed to pages. Push it with: git -C demo/build/pages push origin pages")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", choices=["web", "media", "editor", "publish", "all"])
    parser.add_argument("scenarios", nargs="*", help="scenarios to record with the media command")
    parser.add_argument("--templates", action="store_true", help="rebuild the web export template")
    parser.add_argument("--push", action="store_true", help="push pages after publishing")
    args = parser.parse_args()
    if args.command in ("web", "all"):
        export_web(args.templates)
    if args.command in ("media", "editor", "all"):
        ensure_pages()
    if args.command in ("media", "all"):
        record_media(args.scenarios)
    if args.command in ("editor", "all"):
        take_editor_shots()
    if args.command in ("publish", "all"):
        publish(args.push)


if __name__ == "__main__":
    main()
