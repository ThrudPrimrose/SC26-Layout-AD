"""Auto-switch the DaCe git checkout to the branch a script expects.

Every driver in this experiment runs on ``f2dace/staging`` (E8's
codegen pipeline depends on the f2dace Fortran frontend and on
``StructToContainerGroups``, both shipped by that branch only).
E1..E6 do not import dace at all. E7 (sibling experiment, WIP) pins
``yakup/dev`` -- do not run E7 and E8 concurrently against the same
DaCe clone.

Manual checkout switching is brittle (forget once and you spend an hour
chasing an opaque ``ImportError``), so each script calls
:func:`ensure_branch` *before* importing ``dace``. The helper:

  1. locates the DaCe checkout backing the active Python (the editable
     install location, or ``$DACE_DIR`` / ``$HOME/Work/dace`` fallback),
  2. reads the current HEAD,
  3. switches to the requested branch (and optionally to a pinned
     commit) when needed -- but only if the working tree is clean.

Stdlib-only on purpose: this module must be importable *before*
``import dace`` succeeds, including on a broken DaCe branch where the
package init raises.
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path
from typing import Optional


def _is_git_checkout(path: Path) -> bool:
    """Return True iff ``path`` is the toplevel of a git checkout.

    Accepts regular checkouts and worktrees (``.git`` is a file pointing
    at the gitdir). Rejects paths that merely live inside some other
    git repo -- e.g. ``site-packages`` nested under a pyenv checkout.
    """
    try:
        top = subprocess.check_output(
            ["git", "-C", str(path), "rev-parse", "--show-toplevel"],
            text=True, stderr=subprocess.DEVNULL,
        ).strip()
    except (OSError, subprocess.CalledProcessError):
        return False
    try:
        return Path(top).resolve() == path.resolve()
    except OSError:
        return False


def _locate_dace_repo() -> Optional[Path]:
    """Return the DaCe git checkout backing the active Python install.

    Source of truth is ``pip show dace``'s ``Editable project location``
    -- that's literally where ``import dace`` reads from, including when
    the package init is partially broken on the current branch (so
    ``dace.__file__`` is ``None`` and we can't introspect from inside
    the import).

    ``$DACE_DIR`` overrides for ad-hoc work against a worktree the user
    pip-installed somewhere else.
    """
    env = os.environ.get("DACE_DIR")
    if env and _is_git_checkout(Path(env)):
        return Path(env).resolve()

    try:
        out = subprocess.check_output(
            [sys.executable, "-m", "pip", "show", "dace"],
            text=True, stderr=subprocess.DEVNULL,
        )
    except (OSError, subprocess.CalledProcessError):
        return None
    for line in out.splitlines():
        if line.startswith("Editable project location:"):
            p = Path(line.split(":", 1)[1].strip())
            if _is_git_checkout(p):
                return p.resolve()
        # Fallback for non-editable installs: ``Location:`` points at
        # site-packages; only useful if site-packages is itself a git
        # checkout (rare, but cheap to handle).
        if line.startswith("Location:"):
            p = Path(line.split(":", 1)[1].strip())
            if _is_git_checkout(p):
                return p.resolve()
    return None


def _git(repo: Path, *args: str) -> str:
    return subprocess.check_output(
        ["git", "-C", str(repo), *args], text=True,
    ).strip()


def _working_tree_dirty(repo: Path) -> bool:
    return bool(subprocess.check_output(
        ["git", "-C", str(repo), "status", "--porcelain"], text=True,
    ).strip())


def ensure_branch(
    branch: str,
    *,
    commit: Optional[str] = None,
    allow_drift: bool = False,
) -> None:
    """Ensure the DaCe checkout is on ``branch`` (and optionally
    ``commit``) before this script imports ``dace``.

    On mismatch we ``git checkout`` the requested branch (refusing if
    the tree is dirty so the user's work isn't clobbered). When
    ``commit`` is given and the branch tip differs, we warn -- pass
    ``allow_drift=True`` to suppress, or set ``$DACE_ALLOW_DRIFT=1``.
    """
    repo = _locate_dace_repo()
    if repo is None:
        sys.stderr.write(
            "[dace_branch] could not locate the DaCe git checkout. Set "
            "$DACE_DIR or pip-install DaCe in editable mode against a "
            "checkout under $HOME/Work/dace.\n"
        )
        sys.exit(2)

    try:
        cur_head = _git(repo, "rev-parse", "HEAD")
        cur_branch = _git(repo, "rev-parse", "--abbrev-ref", "HEAD")
    except (OSError, subprocess.CalledProcessError) as e:
        sys.stderr.write(f"[dace_branch] git failed in {repo}: {e}\n")
        sys.exit(2)

    if cur_branch == branch and (commit is None or cur_head == commit):
        print(f"[dace_branch] DaCe @ {repo} already on {branch}"
              + (f"@{cur_head[:12]}" if commit is None else ""))
        return

    if cur_branch != branch:
        if _working_tree_dirty(repo):
            sys.stderr.write(
                f"[dace_branch] DaCe at {repo} has uncommitted changes "
                f"on {cur_branch!r}; refusing to switch to {branch!r}. "
                "Commit or stash first.\n"
            )
            sys.exit(3)
        print(f"[dace_branch] switching DaCe {repo}: {cur_branch} -> {branch}")
        try:
            _git(repo, "checkout", branch)
        except subprocess.CalledProcessError as e:
            sys.stderr.write(f"[dace_branch] git checkout {branch} failed: {e}\n")
            sys.exit(3)
        cur_head = _git(repo, "rev-parse", "HEAD")

    if commit is not None and cur_head != commit:
        msg = (
            f"[dace_branch] {branch} tip is {cur_head[:12]}, expected "
            f"pinned commit {commit[:12]}. f2dace's `_s_<NNN>` counter "
            "and StructToContainerGroups output are revision-sensitive; "
            "regenerated SDFGs may not be byte-stable against committed "
            "C++ headers.\n"
        )
        if allow_drift or os.environ.get("DACE_ALLOW_DRIFT") == "1":
            sys.stderr.write(msg + "  drift allowed, proceeding.\n")
        else:
            sys.stderr.write(
                msg + f"  Refusing to proceed. ``git -C {repo} checkout "
                      f"{commit}`` or rerun with --allow-commit-drift "
                      "(or DACE_ALLOW_DRIFT=1).\n"
            )
            sys.exit(4)
    else:
        print(f"[dace_branch] DaCe @ {repo} now on {branch}@{cur_head[:12]}")


# Pinned f2dace/staging tip the experiment was developed against. Re-pin
# only after re-running the full pipeline (f90_to_sdfg + generate_baselines
# + stages 1-6) end-to-end. This is the only branch the artifact uses.
F2DACE_BRANCH = "f2dace/staging"
F2DACE_COMMIT = "8d1654b41fd854addb171f354b944ea373954eb3"
