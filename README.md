# boxe demo

The demo project of [boxe](https://github.com/ponttorez/boxe/tree/boxe), a fork of Godot Engine. It shows the features
of the fork next to stock Godot, and runs in the browser at [ponttorez.github.io/boxe](https://ponttorez.github.io/boxe/).

It needs the editor of boxe. In a checkout of the `boxe` branch, this branch goes into the `demo` folder:

```bash
git fetch origin demo
```

```bash
git worktree add demo demo
```

[`tools/media.py`](tools/media.py) builds the web demo and records the GIFs and screenshots of the README of boxe.
The video at the top of that README is the [`tools/motion`](tools/motion) scene: a banner, then each problem of stock
Godot next to its fix, recorded at 1920×1080.
