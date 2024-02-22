"""Rename Python wheel to contain commit ID"""

import argparse
import pathlib


def main(args: argparse.Namespace) -> None:
    """Rename Python wheel"""
    wheel_dir = pathlib.Path(args.wheel_dir).expanduser().resolve()
    if not wheel_dir.is_dir():
        raise ValueError("wheel_dir argument must be a directory")

    for whl_path in wheel_dir.glob("*.whl"):
        basename = whl_path.name
        tokens = basename.split("-")
        assert len(tokens) == 5
        keywords = {
            "pkg_name": tokens[0],
            "version": tokens[1],
            "commit_id": args.commit_id,
            "platform_tag": args.platform_tag,
        }
        new_name = (
            "{pkg_name}-{version}+{commit_id}-py3-none-{platform_tag}.whl".format(
                **keywords
            )
        )
        new_path = whl_path.parent / new_name
        print(f"Renaming {whl_path} to {new_path}...")
        whl_path.rename(whl_path.parent / new_path)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=(
            "Script to rename wheel(s) using a commit ID and platform tag."
            "Note: This script will not recurse into subdirectories."
        )
    )
    parser.add_argument("wheel_dir", type=str, help="Directory containing wheels")
    parser.add_argument("commit_id", type=str, help="Hash of current git commit")
    parser.add_argument(
        "platform_tag", type=str, help="Platform tag, PEP 425 compliant"
    )
    parsed_args = parser.parse_args()
    main(parsed_args)
