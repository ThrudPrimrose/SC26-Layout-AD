"""Produce the main-compatible baseline SDFG for the VelocityTendenciesPipeline repo.

Runs stage 1's optimisation action (AoS->SoA + hygiene + add_symbols, minus
hardcoded-name reductions) on the un-specialised velocity_no_nproma.sdfgz
straight through, without splitting into the 4 (lvn_only, istep) variants.

Must run under the old f2dace/staging DaCe branch. Output is loadable by
mainline DaCe.
"""
import argparse
from pathlib import Path

import dace

from utils.stages.compile_gpu_stage1 import optimization_action


def main():
    argp = argparse.ArgumentParser()
    argp.add_argument("--input", default="velocity_no_nproma.sdfgz")
    argp.add_argument("--output", default="baseline/velocity_no_nproma_post_aos_soa.sdfgz")
    args = argp.parse_args()

    sdfg = dace.SDFG.from_file(args.input)
    sdfg.validate()
    sdfg = optimization_action(sdfg)

    out = Path(args.output)
    out.parent.mkdir(parents=True, exist_ok=True)
    sdfg.save(out, compress=True)
    print(f"Baseline saved to {out}")


if __name__ == "__main__":
    main()
