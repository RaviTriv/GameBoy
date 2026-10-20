import argparse
import hashlib
import os
import subprocess
import tarfile
import tempfile
import urllib.request

DEPS = [
    {
        "name": "fmt",
        "url": "https://github.com/fmtlib/fmt/archive/refs/tags/12.2.0.tar.gz",
        "sha256": "8b852bb5aa6e7d8564f9e81394055395dd1d1936d38dfd3a17792a02bebd7af0",
        "cmake_args": ["-DFMT_TEST=OFF", "-DFMT_DOC=OFF"],
    },
    {
        "name": "spdlog",
        "url": "https://github.com/gabime/spdlog/archive/refs/tags/v1.17.0.tar.gz",
        "sha256": "d8862955c6d74e5846b3f580b1605d2428b11d97a410d86e2fb13e857cd3a744",
        "cmake_args": ["-DSPDLOG_FMT_EXTERNAL=ON", "-DSPDLOG_BUILD_EXAMPLE=OFF"],
    },
]


def run(*cmd, **kwargs):
    print("+", " ".join(cmd), flush=True)
    subprocess.run(cmd, check=True, **kwargs)


def apt_install(packages):
    env = {**os.environ, "DEBIAN_FRONTEND": "noninteractive"}
    run("apt-get", "update", env=env)
    run("apt-get", "install", "-y", "--no-install-recommends", *packages, env=env)


def fetch(dep, workdir):
    archive = os.path.join(workdir, f"{dep['name']}.tar.gz")
    with urllib.request.urlopen(dep["url"]) as response, open(archive, "wb") as out:
        out.write(response.read())

    with open(archive, "rb") as f:
        digest = hashlib.sha256(f.read()).hexdigest()
    if digest != dep["sha256"]:
        raise SystemExit(f"{dep['name']}: sha256 mismatch, expected {dep['sha256']}, got {digest}")

    with tarfile.open(archive) as tar:
        top = tar.getnames()[0].split("/")[0]
        tar.extractall(workdir, filter="data")
    return os.path.join(workdir, top)


def build_and_install(dep, workdir):
    source = fetch(dep, workdir)
    build = os.path.join(workdir, f"{dep['name']}-build")
    run("cmake", "-S", source, "-B", build, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release", *dep["cmake_args"])
    run("cmake", "--build", build)
    run("cmake", "--install", build)


def main():
    parser = argparse.ArgumentParser(description="Install pinned CI dependencies")
    parser.add_argument("apt_packages", nargs="*", help="Extra apt packages to install")
    args = parser.parse_args()

    apt_install(["cmake", "ninja-build", *args.apt_packages])

    with tempfile.TemporaryDirectory() as workdir:
        for dep in DEPS:
            build_and_install(dep, workdir)


if __name__ == "__main__":
    main()
