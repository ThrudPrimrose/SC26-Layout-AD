"""E8 (Legacy VT) compile driver.

Ported from E7's ``utils/compile_if_propagated_sdfgs.py`` so the flag set is
byte-identical with E7 / E0-E6. The two extra legacy kwargs
``allocation_names_to_comment_out`` and ``use_openacc_stream`` are kept in
the public signature as no-ops so the legacy stage drivers (which still
pass them) link without modification.

The legacy stage drivers in ``utils/stages/compile_gpu_stage{1..9}.py``
remain in charge of SDFG-side codegen choices; only the post-codegen
flag set has been canonicalised here.
"""

import os

# Pin ``__DACE_NO_SYNC=1`` BEFORE importing ``dace`` so the codegen's
# ``cudaStreamSynchronize`` / ``cudaDeviceSynchronize`` /
# ``cudaEventSynchronize`` emission sites (gated on
# ``dace.codegen.common.no_sync_emission()``) all elide. The legacy
# pipeline uses explicit per-state sync tasklets in
# ``OffloadVelocityToGPU`` / ``pre_gpu_fixes`` instead. Callers that want
# sync emission back can set ``__DACE_NO_SYNC=0`` before the first
# import.
os.environ.setdefault("__DACE_NO_SYNC", "1")

import dace
import sys
import typing
from dace.sdfg import infer_types


AMD = (
    os.getenv("__HIP_PLATFORM_AMD__", "0") == "1"
    or os.getenv("HIP_PLATFORM_AMD", "0") == "1"
)

USE_NVHPC = os.getenv("_USE_NVHPC", "0").lower() in ("1", "true", "yes")


# ─── File path helpers ────────────────────────────────────────────────
def _cpu_src(build_loc: str, name: str) -> str:
    return f"{build_loc}/src/cpu/{name}.cpp"


def _gpu_src(build_loc: str, name: str) -> str:
    if AMD:
        return f"{build_loc}/src/cuda/hip/{name}_cuda.cpp"
    return f"{build_loc}/src/cuda/{name}_cuda.cu"


def _header(build_loc: str, name: str) -> str:
    return f"{build_loc}/include/{name}.h"


# ─── Build helpers ────────────────────────────────────────────────────
def _cxx() -> str:
    return os.getenv("CXX") or ("hipcc" if AMD else "g++")


def _nvcc() -> str:
    return "hipcc" if AMD else "nvcc"


def _pick_compiler(src: str) -> str:
    return _nvcc() if src.endswith(".cu") else _cxx()


def _get_link_compiler(gpu: bool) -> str:
    if not gpu:
        return _cxx()
    return "hipcc" if AMD else "nvcc"


def _get_flags(gpu: bool, release: bool, lib: bool, debuginfo: bool) -> str:
    if gpu and AMD:
        # Match setup_beverin.sh's GPU_CXXFLAGS: pin libgomp explicitly so
        # the binary links against /usr/lib/.../libgomp.so (which the
        # cluster has) instead of LLVM's libomp.so (which it does not),
        # otherwise the runtime fails with
        # ``error while loading shared libraries: libomp.so: cannot
        # open shared object file: No such file or directory``.
        omp_flag = "-fopenmp=libgomp"
    elif gpu:
        omp_flag = "-Xcompiler=-fopenmp"
    else:
        omp_flag = "-fopenmp"

    if gpu and AMD:
        arch = "gfx942"
        # Pin ROCm + HIP install paths explicitly on the command line.
        # With ``-x hip`` on .cpp/.cu, ROCm clang++ otherwise probes for
        # a CUDA install and fails (``cannot find libdevice for sm_35``)
        # whenever the ``ROCM_PATH`` / ``HIP_PATH`` env propagation is
        # lost in a subprocess (LLVM #63660; ROCm/HIP #1716). Honor
        # ROCM_HOME / ROCM_PATH and HIP_PATH (set by setup_beverin.sh)
        # but also bake them into the flags so a stripped-env caller
        # can't accidentally fall back to the CUDA code path.
        rocm_path = os.getenv("ROCM_HOME") or os.getenv("ROCM_PATH") or "/opt/rocm"
        hip_path  = os.getenv("HIP_PATH") or rocm_path
        common = (
            f"--rocm-path={rocm_path} --hip-path={hip_path} "
            f"--offload-arch={arch} -std=c++20 -DNDEBUG"
        )
        if release:
            flags = (
                f"{common} -Wall -Wextra {omp_flag} "
                f"--offload-arch={arch} "
                "-mllvm -amdgpu-early-inline-all=true "
                "-munsafe-fp-atomics "
                "-ffp-contract=fast "
                "-fPIC -O3 -ffast-math "
                "-Wno-unused-parameter -Wno-ignored-attributes -Wno-unused-result"
            )
        else:
            flags = (
                f"{common} -O0 -g -ggdb -fPIC -Wall -Wextra {omp_flag} "
                "-Wno-unused-parameter -Wno-ignored-attributes -Wno-unused-result "
                "-DDACE_VELOCITY_DEBUG"
            )
        if debuginfo:
            flags += " -g"
        if lib:
            flags += " -DNO_SERDE -fPIC -shared"
    elif gpu:
        gencode_num = os.getenv("GENCODE_NUMBER", "90a")
        if gencode_num == "0":
            raise ValueError("Set GENCODE_NUMBER (e.g. 90)")
        nvhpc = "-ccbin=nvc++ " if USE_NVHPC else ""
        suppress = (
            "--diag-suppress 68 --diag-suppress 550 --diag-suppress 20208 "
            "--diag-suppress 1835 --diag-suppress 177 --diag-suppress 20012 "
            "--diag-suppress 1098"
        )
        xcompiler_warns = (
            "-Xcompiler=-Wconversion -Xcompiler=-Wsign-conversion "
            "-Xcompiler=-Wfloat-conversion -Xcompiler=-Wno-unknown-pragmas "
            "-Xcompiler=-faligned-new"
        ) if not USE_NVHPC else ""
        debugflag = "-lineinfo" if debuginfo else ""
        arch_flag = f"-arch=sm_{gencode_num}"

        if release:
            flags = (
                f"{nvhpc}{suppress} {xcompiler_warns} -DNDEBUG -Xcompiler=-DNDEBUG "
                f"-Xcompiler=-Wall -Xcompiler=-Wextra -Xcompiler=-O3 {omp_flag} "
                f"--expt-relaxed-constexpr {arch_flag} "
                f"--use_fast_math -O3 {debugflag} --ftz=true "
                f"--prec-div=false --prec-sqrt=false --fmad=true "
                f"-Xptxas=-O3 -Xptxas=-v -Xcompiler=-march=native "
                f"-Xcompiler=-mtune=native --restrict"
            )
        else:
            flags = (
                f"{suppress} {xcompiler_warns} "
                f"-Xcompiler=-Wall -Xcompiler=-Wextra {omp_flag} "
                f"--expt-relaxed-constexpr {arch_flag} "
                f"-O0 -Xcompiler=-g -g -Xcompiler=-O0 -G {debugflag} "
                f"--fmad=false --prec-div=true --prec-sqrt=true --ftz=false "
                f"-DDACE_VELOCITY_DEBUG -Xcompiler=-DDACE_VELOCITY_DEBUG"
            )
        if lib:
            flags += " -DNO_SERDE -std=c++17 -Xcompiler=-fPIC --compiler-options '-fPIC' --shared"
        else:
            flags += " -std=c++20"
    else:
        warns = (
            "-Wconversion -Wno-sign-conversion -Wfloat-conversion "
            "-Wno-unknown-pragmas -faligned-new"
        ) if not USE_NVHPC else ""
        debugflag = "-g" if debuginfo else ""
        if release:
            flags = (
                f"{warns} {debugflag} -std=c++20 -Wall -Wextra "
                f"-Wno-unused-parameter -Wno-unused-variable -O3 -DNDEBUG "
                f"{omp_flag}"
            )
        else:
            flags = (
                f"{warns} -DDACE_VELOCITY_DEBUG -std=c++20 -Wall -Wextra "
                f"-Wno-unused-parameter -Wno-unused-variable "
                f"-Wno-unknown-pragmas -O0 -g -ggdb {debugflag} "
                f"{omp_flag}"
            )

    if AMD:
        flags += " -D__HIP_PLATFORM_AMD__=1"
        flags += " -DHIP_PLATFORM_AMD=1"

    return flags


import subprocess
from concurrent.futures import ThreadPoolExecutor, as_completed


def _gcc_libgomp_path() -> typing.Optional[str]:
    """Absolute path to gcc's ``libgomp.so.1``, or None if gcc isn't on
    PATH or the library can't be located. Used by the link step on AMD
    to pin libgomp explicitly (mirror of what
    ``setup_beverin.sh:GPU_CXXFLAGS`` does for E1-E6 via
    ``-fopenmp=libgomp`` in a single hipcc command). Resolving via
    ``gcc -print-file-name`` is the same source-of-truth E1-E6 use
    transitively when their hipcc-run gcc preprocessor lookup picks
    libgomp; doing it explicitly here keeps the propagated-SDFG link
    line robust even when subprocess env propagation strips
    ``LD_LIBRARY_PATH`` or the spack-loaded gcc's run-time deps.
    """
    try:
        out = subprocess.check_output(
            ["gcc", "-print-file-name=libgomp.so.1"],
            text=True, stderr=subprocess.DEVNULL,
        ).strip()
        if out and os.path.isfile(out):
            return out
    except (subprocess.CalledProcessError, FileNotFoundError, OSError):
        pass
    return None


def _compile_and_link(
    sources: typing.List[str],
    includes: str,
    flags: str,
    output: str,
    gpu: bool,
    lib: bool,
    jobs: int = os.cpu_count(),
    sdfg_names: typing.Optional[typing.List[str]] = None,
):
    compile_flags = flags.replace("--shared", "").replace("-shared", "")
    import re as _re
    _NVCC_ONLY_TOKENS = _re.compile(
        r"(?:--diag-suppress\s+\S+|"
        r"-arch=sm_\w+|"
        r"--expt-relaxed-constexpr|"
        r"--use_fast_math|"
        r"--ftz=\S+|--prec-div=\S+|--prec-sqrt=\S+|--fmad=\S+|"
        r"-Xptxas=\S+|"
        r"--restrict|"
        r"-lineinfo|"
        r"-G(?=\s|$)|"
        r"-ccbin=\S+)"
    )
    _XCOMPILER = _re.compile(r"-Xcompiler=(\S+)")

    def _flags_for_cxx(f: str) -> str:
        f = _NVCC_ONLY_TOKENS.sub("", f)
        f = _XCOMPILER.sub(r"\1", f)
        return " ".join(f.split())

    objects = []

    def compile_one(src):
        obj = os.path.splitext(src)[0] + ".o"
        obj_dir = os.path.dirname(obj)
        if obj_dir:
            os.makedirs(obj_dir, exist_ok=True)
        xlang = ""
        if gpu and src.endswith((".cpp", ".cu")):
            cc = _nvcc()
            # Language-flag dispatch:
            #   * AMD: hipcc (and underlying ROCm clang++) defaults
            #     ``.cu`` files to CUDA mode. ``-x hip`` is required
            #     for BOTH ``.cpp`` and ``.cu`` to keep clang++ on the
            #     ROCm code path; without it we get
            #     ``cannot find libdevice for sm_35`` / ``cannot find
            #     CUDA installation``.
            #   * NVIDIA + ``.cpp``: nvcc needs explicit ``-x cu`` to
            #     interpret the source as CUDA.
            #   * NVIDIA + ``.cu``: that's nvcc's default; no ``-x``.
            if AMD:
                xlang = "-x hip"
            elif src.endswith(".cpp"):
                xlang = "-x cu"
        else:
            cc = _pick_compiler(src)
        per_flags = (compile_flags if cc.endswith("nvcc") or cc.endswith("hipcc")
                     else _flags_for_cxx(compile_flags))
        cmd = f"{cc} {xlang} -c {src} {includes} {per_flags} -o {obj}"
        ret = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        return src, obj, cc, ret

    with ThreadPoolExecutor(max_workers=jobs) as pool:
        futures = {pool.submit(compile_one, src): src for src in sources}
        for fut in as_completed(futures):
            src, obj, cc, ret = fut.result()
            print(f"  [CC {cc}] {src}")
            if ret.stdout:
                print(ret.stdout, end="")
            if ret.stderr:
                print(ret.stderr, end="", file=sys.stderr)
            if ret.returncode != 0:
                raise RuntimeError(f"FAILED: {src}")
            objects.append(obj)

    link_cc = _get_link_compiler(gpu)

    import re as _re
    arch_match = _re.search(r"-arch=sm_\w+", compile_flags)
    arch_flag = arch_match.group(0) if arch_match else ""

    if gpu and sdfg_names:
        objects = _localize_internal_symbols(objects, sdfg_names)

    link_flags = ""
    if lib:
        link_flags = "-shared" if (AMD or link_cc == _cxx()) else "--shared -Xcompiler=-fPIC"

    if "nvcc" in link_cc:
        link_flags += " -Xcompiler=-fopenmp "
    elif AMD:
        # libgomp pin -- four layers of defense against the
        # ``libomp.so: cannot open shared object file`` runtime error
        # on Beverin (clang/hipcc's default OpenMP runtime is libomp,
        # which isn't installed on the cluster):
        #   1. ``-fopenmp=libgomp`` tells clang's driver to select
        #      libgomp for OpenMP at compile + link time;
        #   2. The absolute path to gcc's libgomp.so.1 is added as a
        #      positional arg so the linker can't fall back to lib
        #      search heuristics. DT_NEEDED ends up with the canonical
        #      ``libgomp.so.1`` SONAME, never ``libomp.so``;
        #   3. ``-Wl,-rpath=<dir>`` bakes that directory into the
        #      binary's RUNPATH so the loader finds libgomp at run
        #      time without LD_LIBRARY_PATH cooperation;
        #   4. ``-Wl,--as-needed`` drops DT_NEEDED for any library
        #      that hipcc's driver might still inject implicitly but
        #      whose symbols never contribute (so even if ``-lomp``
        #      sneaks onto the link line, libomp.so doesn't appear in
        #      the binary's deps).
        # Falls back to bare ``-lgomp`` if gcc isn't on PATH.
        gomp_path = _gcc_libgomp_path()
        if gomp_path:
            # Order matters: ``-Wl,--as-needed`` and ``-Wl,-rpath`` must
            # appear BEFORE any ``-l*`` / positional library path so the
            # linker applies them to the libs that follow. The libgomp
            # absolute path is placed last so it lands at the rightmost
            # of the link line (after objects), satisfying the
            # libraries-after-objects rule for symbol resolution.
            link_flags += (
                f" -fopenmp=libgomp "
                f"-Wl,-rpath={os.path.dirname(gomp_path)} "
                f"-Wl,--as-needed {gomp_path} "
            )
        else:
            link_flags += " -fopenmp=libgomp -Wl,--as-needed -lgomp "
    else:
        link_flags += " -fopenmp "

    link_cmd = f"{link_cc} {' '.join(objects)} {arch_flag} {link_flags} -o {output}"
    print(f"  [LD {link_cc}] {output}")
    ret = subprocess.run(link_cmd, shell=True)
    if ret.returncode != 0:
        raise RuntimeError(f"FAILED: {link_cmd}")


def _localize_internal_symbols(
    objects: typing.List[str], sdfg_names: typing.List[str],
) -> typing.List[str]:
    """Per-SDFG: partial-link the ``.cpp.o`` + ``_cuda.o`` into
    ``<sdfg>_merged.o``, then ``objcopy --localize-symbol`` to downgrade
    only the symbols that COLLIDE across variants. See E7's twin for
    the full rationale."""
    if len(sdfg_names) <= 1:
        return list(objects)

    merged_paths: typing.Dict[str, str] = {}
    for sdfg_name in sdfg_names:
        cpu_obj = next(
            (o for o in objects
             if o.endswith(f'/{sdfg_name}.o') and '/src/cpu/' in o),
            None)
        cuda_obj = next(
            (o for o in objects
             if o.endswith(f'/{sdfg_name}_cuda.o') and '/src/cuda/' in o),
            None)
        if cpu_obj is None or cuda_obj is None:
            continue
        merged = os.path.join(os.path.dirname(cpu_obj),
                               f'{sdfg_name}_merged.o')
        ld_cmd = f"ld -r {cpu_obj} {cuda_obj} -o {merged}"
        ret = subprocess.run(ld_cmd, shell=True)
        if ret.returncode != 0:
            raise RuntimeError(f"FAILED: {ld_cmd}")
        merged_paths[sdfg_name] = merged

    import collections as _collections
    sym_count = _collections.Counter()
    sym_per_obj: typing.Dict[str, typing.Set[str]] = {}
    for sdfg_name, merged in merged_paths.items():
        defined = set()
        nm = subprocess.run(f'nm --defined-only {merged}', shell=True,
                             capture_output=True, text=True)
        for line in (nm.stdout or '').splitlines():
            parts = line.strip().split(None, 2)
            if len(parts) >= 3 and parts[1] in ('T', 'D', 'B', 'R'):
                defined.add(parts[2])
        sym_per_obj[sdfg_name] = defined
        for s in defined:
            sym_count[s] += 1

    colliders = {s for s, c in sym_count.items() if c >= 2}
    if not colliders:
        out = list(objects)
        for sdfg_name, merged in merged_paths.items():
            cpu_obj = next(o for o in objects
                            if o.endswith(f'/{sdfg_name}.o')
                            and '/src/cpu/' in o)
            cuda_obj = next(o for o in objects
                            if o.endswith(f'/{sdfg_name}_cuda.o')
                            and '/src/cuda/' in o)
            out = [o for o in out if o != cpu_obj and o != cuda_obj]
            out.append(merged)
        return out

    for sdfg_name, merged in merged_paths.items():
        local_set = colliders & sym_per_obj[sdfg_name]
        if not local_set:
            continue
        listfile = merged + '.localize.txt'
        with open(listfile, 'w') as f:
            for s in sorted(local_set):
                f.write(s + '\n')
        objcopy_cmd = f"objcopy --localize-symbols={listfile} {merged}"
        ret = subprocess.run(objcopy_cmd, shell=True)
        if ret.returncode != 0:
            raise RuntimeError(f"FAILED: {objcopy_cmd}")

    out = list(objects)
    for sdfg_name, merged in merged_paths.items():
        cpu_obj = next(o for o in objects
                        if o.endswith(f'/{sdfg_name}.o')
                        and '/src/cpu/' in o)
        cuda_obj = next(o for o in objects
                        if o.endswith(f'/{sdfg_name}_cuda.o')
                        and '/src/cuda/' in o)
        out = [o for o in out if o != cpu_obj and o != cuda_obj]
        out.append(merged)
    return out


# ─── Main entry point ────────────────────────────────────────────────
def compile_if_propagated_sdfgs(
    sdfgs: typing.List[dace.SDFG],
    gpu: bool,
    release: bool,
    generate_code: bool,
    lib: bool,
    main_name: typing.Optional[str],
    stage: int,
    debuginfo: bool,
    extra_sources: typing.Optional[typing.Iterable[str]] = None,
    extra_include_dirs: typing.Optional[typing.Iterable[str]] = None,
    output: typing.Optional[str] = None,
    post_codegen_hook: typing.Optional[
        typing.Callable[[typing.List[dace.SDFG]], None]
    ] = None,
    # ── Legacy kwargs accepted for drop-in compat with the legacy stage
    #    drivers (utils/stages/compile_gpu_stage{1..9}.py). They were used
    #    by the old in-tree source-patching pass that E7 dropped; the
    #    velocity SDFGs do not need them. Accepted and ignored.
    allocation_names_to_comment_out: typing.Any = None,
    use_openacc_stream: typing.Any = None,
):
    """Codegen each SDFG and compile + link with ``main_name``.

    Flag set is byte-identical with E7's
    ``utils/compile_if_propagated_sdfgs.py``.
    """
    flags = _get_flags(gpu, release, lib, debuginfo)

    dace.Config.set("compiler", "cuda", "max_concurrent_streams", value="1")

    # Pin the legacy CUDA codegen. The new-gpu-codegen-dev branch ships
    # an ``experimental`` codegen that's the default but is producing
    # wrong numerics on the velocity SDFGs; ``legacy`` is the stable
    # path. Guarded for older DaCe branches that don't have the key.
    try:
        dace.Config.set("compiler", "cuda", "implementation", value="legacy")
    except (KeyError, ValueError):
        pass

    block_dims = os.getenv("_TBLOCK_DIMS", "32,16,1")
    dace.Config.set("compiler", "cuda", "default_block_size", value=block_dims)

    if gpu and AMD:
        dace.Config.set("compiler", "cuda", "backend", value="hip")
        dace.Config.set("compiler", "cuda", "path", value="/opt/rocm")
        dace.Config.set("compiler", "cuda", "hip_arch", value="gfx942")
        dace.Config.set("compiler", "cuda", "hip_args", value=flags)
    elif gpu:
        dace.Config.set("compiler", "cuda", "args", value=flags)

    cpu_flags = _get_flags(False, release, lib, debuginfo)
    dace.Config.set("compiler", "cpu", "args", value=cpu_flags)

    sources: typing.List[str] = list(extra_sources) if extra_sources is not None else []
    headers = [f"-I{d}" for d in (extra_include_dirs or ["include"])]

    from dace.codegen import codegen, compiler

    for sdfg in sdfgs:
        name = sdfg.name
        build_loc = sdfg.build_folder

        if generate_code:
            try:
                sdfg.fill_scope_connectors()
                infer_types.infer_connector_types(sdfg)
                infer_types.set_default_schedule_and_storage_types(sdfg, None)
                sdfg.expand_library_nodes()
                infer_types.infer_connector_types(sdfg)
                infer_types.set_default_schedule_and_storage_types(sdfg, None)
                program_objects = codegen.generate_code(sdfg, validate=False)
            except Exception:
                fpath = os.path.join("_dacegraphs", "failing.sdfgz")
                os.makedirs("_dacegraphs", exist_ok=True)
                sdfg.save(fpath, compress=True)
                print(f"Failing SDFG saved: {os.path.abspath(fpath)}")
                raise

            compiler.generate_program_folder(sdfg, program_objects, build_loc)

        cpu_file = _cpu_src(build_loc, name)
        gpu_file = _gpu_src(build_loc, name)

        sources.append(cpu_file)
        if gpu:
            sources.append(gpu_file)

    # Link the E8 reductions implementation. The header
    # ``include/reductions_kernel.cuh`` declares the host-side
    # ``reduce_*_gpu`` symbols; the body lives in:
    #   * ``src/reductions_kernel.cu``  -- CUDA path; ``#else`` branch
    #     guarded by ``__HIP_PLATFORM_AMD__`` selects ``cub::DeviceReduce``
    #     and ``kernel<<<...>>>`` syntax.
    #   * ``src/reductions_kernel.cpp`` -- HIP path; trimmed to the
    #     ``__HIP_PLATFORM_AMD__`` branch only so hipcc doesn't try to
    #     parse the ``#else`` CUDA-syntax kernel launches.
    # The CPU-side ``reduce_*_cpu`` impls live in ``src/reductions.cpp``
    # and are linked unconditionally (called from the host glue under
    # ``codegen/.../src/cpu/*.cpp``).
    if gpu:
        _e8_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        _gpu_red = "reductions_kernel.cpp" if AMD else "reductions_kernel.cu"
        sources.append(os.path.join(_e8_root, "src", _gpu_red))
        sources.append(os.path.join(_e8_root, "src", "reductions.cpp"))

    if post_codegen_hook is not None:
        post_codegen_hook(sdfgs)

    if main_name is not None:
        sources.append(main_name)
    elif not lib:
        sources.append("main_gpu.cpp" if gpu else "main.cpp")

    dace_include = os.path.dirname(dace.__file__) + "/runtime/include/"
    includes = " ".join(
        [f"-I{sdfg.build_folder}/include" for sdfg in sdfgs]
        + [f"-I{dace_include}"]
        + headers
    )

    if output is None:
        if lib:
            output = "libvelocity_gpu.so" if gpu else "libvelocity_cpu.so"
        else:
            output = "velocity_gpu" if gpu else "velocity_cpu"

    print(f"\nBackend: {'HIP (AMD)' if AMD else 'CUDA (NVIDIA)' if gpu else 'CPU'}")
    print(f"CXX: {_cxx()}  nvcc: {_nvcc() if gpu else '(unused)'}")
    print(f"Output: {output}")
    print(f"Sources: {len(sources)} files\n")

    _compile_and_link(sources, includes, flags, output, gpu, lib,
                      sdfg_names=[s.name for s in sdfgs])
