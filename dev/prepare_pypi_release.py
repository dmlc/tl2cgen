"""Simple script for preparing a PyPI release.
It fetches Python wheels from the CI pipelines.
tqdm, packaging are required to run this script.
"""

import argparse
import pathlib
import subprocess
from typing import List, Optional
from urllib.request import urlretrieve

import tqdm
from packaging import version

PREFIX = "https://tl2cgen-wheels.s3.amazonaws.com/"
PROJECT_ROOT = pathlib.Path(__file__).expanduser().resolve().parent.parent
DIST_DIR = PROJECT_ROOT / "python" / "dist"


pbar = None  # pylint: disable=invalid-name


def show_progress(block_num, block_size, total_size):
    """Show file download progress."""
    global pbar  # pylint: disable=global-statement
    if pbar is None:
        pbar = tqdm.tqdm(total=total_size / 1024, unit="kB")

    downloaded = block_num * block_size
    if downloaded < total_size:
        upper = (total_size - downloaded) / 1024
        pbar.update(min(block_size / 1024, upper))
    else:
        pbar.close()
        pbar = None


def retrieve(url, filename=None):
    """Download a file from URL"""
    print(f"{url} -> {filename}")
    return urlretrieve(url, filename, reporthook=show_progress)


def latest_hash() -> str:
    """Get latest commit hash."""
    ret = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True, check=True)
    assert ret.returncode == 0, "Failed to get latest commit hash."
    commit_hash = ret.stdout.decode("utf-8").strip()
    return commit_hash


def download_wheels(
    platforms: List[str],
    url_prefix: str,
    dest_dir: pathlib.Path,
    src_filename_prefix: str,
    target_filename_prefix: str,
    ext: str = "whl",
) -> List[str]:
    """Download all binary wheels. url_prefix is the URL for remote directory storing
    the release wheels
    """
    # pylint: disable=too-many-arguments

    filenames = []
    for platform in platforms:
        src_wheel = src_filename_prefix + platform + "." + ext
        url = url_prefix + src_wheel

        target_wheel = target_filename_prefix + platform + "." + ext
        print(f"{src_wheel} -> {target_wheel}")
        filename = dest_dir / target_wheel
        filenames.append(str(filename))
        retrieve(url=url, filename=filename)
    return filenames


def download_py_packages(version_str: str, commit_hash: str) -> None:
    """Download Python packages"""
    platforms = [
        "win_amd64",
        "manylinux2014_x86_64",
        "macosx_10_15_x86_64.macosx_11_0_x86_64.macosx_12_0_x86_64",
        "macosx_12_0_arm64",
    ]

    if not DIST_DIR.exists():
        DIST_DIR.mkdir()

    # Binary wheels (*.whl)
    src_filename_prefix = f"tl2cgen-{version_str}%2B{commit_hash}-py3-none-"
    target_filename_prefix = f"tl2cgen-{version_str}-py3-none-"
    filenames = download_wheels(
        platforms, PREFIX, DIST_DIR, src_filename_prefix, target_filename_prefix
    )
    print(f"List of downloaded wheels: {filenames}\n")

    # Source distribution (*.tar.gz)
    src_filename_prefix = f"tl2cgen-{version_str}%2B{commit_hash}"
    target_filename_prefix = f"tl2cgen-{version_str}"
    filenames = download_wheels(
        [""],
        PREFIX,
        DIST_DIR,
        src_filename_prefix,
        target_filename_prefix,
        "tar.gz",
    )
    print(f"List of downloaded sdist: {filenames}\n")
    print(
        """
Following steps should be done manually:
- Upload pypi package by `python -m twine upload python/dist/*` for all wheels.
- Check the uploaded files on `https://pypi.org/project/tl2cgen/<VERSION>/#files` and `pip
  install tl2cgen==<VERSION>` """
    )


def main(args: argparse.Namespace) -> None:
    """Entry function"""
    rel = version.parse(args.release)
    assert isinstance(rel, version.Version)

    if not rel.is_prerelease:
        # Major release
        rc: Optional[str] = None
    else:
        # RC release
        assert rel.pre is not None
        rc, _ = rel.pre
        assert rc == "rc"

    commit_hash = latest_hash()

    download_py_packages(args.release, commit_hash)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--release",
        type=str,
        required=True,
        help="Version tag, e.g. '1.3.2', or '1.5.0rc1'",
    )
    parsed_args = parser.parse_args()
    main(parsed_args)
