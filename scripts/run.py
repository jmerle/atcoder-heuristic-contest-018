import json
import multiprocessing
import os
import shutil
import subprocess
import tempfile
from argparse import ArgumentParser
from contextlib import contextmanager
from pathlib import Path
from typing import Generator, List

PROJECT_ROOT = Path(__file__).parent.parent

SOLVER_BINARY_ROOT = PROJECT_ROOT / "cmake-build-release"

RESULTS_DIRECTORY = PROJECT_ROOT / "results"
INPUT_DIRECTORY = RESULTS_DIRECTORY / "in"
OUTPUT_DIRECTORY = RESULTS_DIRECTORY / "out"
OVERVIEW_FILE = RESULTS_DIRECTORY / "overview.html"

SCRIPTS_DIRECTORY = PROJECT_ROOT / "scripts"
GEN_BINARY = SCRIPTS_DIRECTORY / "gen"
TESTER_BINARY = SCRIPTS_DIRECTORY / "tester"
VIS_BINARY = SCRIPTS_DIRECTORY / "vis"
OVERVIEW_TEMPLATE_FILE = SCRIPTS_DIRECTORY / "overview.tmpl.html"

@contextmanager
def temporary_file() -> Generator[Path, None, None]:
    fd, name = tempfile.mkstemp()
    os.close(fd)

    path = Path(name)

    try:
        yield path
    finally:
        path.unlink()

@contextmanager
def temporary_directory() -> Generator[Path, None, None]:
    path = Path(tempfile.mkdtemp())

    try:
        yield path
    finally:
        shutil.rmtree(path)

def get_score(err_file: Path) -> int:
    content = err_file.read_text(encoding="utf-8").strip()
    total_cost_line = next((line for line in content.splitlines() if line.startswith("Total Cost = ")), None)
    return int(total_cost_line.split(" = ")[1]) if total_cost_line is not None else 0

def update_overview() -> None:
    scores_by_solver = {}

    for directory in OUTPUT_DIRECTORY.iterdir():
        scores_by_seed = {}

        for file in directory.iterdir():
            if file.name.endswith(".err"):
                scores_by_seed[file.stem] = get_score(file)

        scores_by_solver[directory.name] = scores_by_seed

    overview = OVERVIEW_TEMPLATE_FILE.read_text(encoding="utf-8")
    overview = overview.replace("/* scores_by_solver */{}", json.dumps(scores_by_solver))

    with OVERVIEW_FILE.open("w+", encoding="utf-8") as file:
        file.write(overview)

    print(f"Overview: file://{OVERVIEW_FILE}")

def generate_seed(seed: int) -> None:
    seed_file = INPUT_DIRECTORY / f"{seed}.txt"
    if seed_file.is_file():
        return

    if not seed_file.parent.is_dir():
        seed_file.parent.mkdir(parents=True)

    with temporary_file() as tmp_file, temporary_directory() as tmp_directory:
        tmp_file.write_text(f"{seed}\n", encoding="utf-8")

        process = subprocess.run([str(GEN_BINARY), str(tmp_file)],
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT,
                                 cwd=str(tmp_directory))

        if process.returncode != 0:
            raise RuntimeError(f"Generator exited with status code {process.returncode} for seed {seed}:\n{process.stdout.decode('utf-8').strip()}")

        seed_file.write_text((tmp_directory / "in" / "0000.txt").read_text(encoding="utf-8"), encoding="utf-8")

def run_seed(solver: Path, seed: int, solver_output_directory: Path) -> int:
    out_file = solver_output_directory / f"{seed}.out"
    err_file = solver_output_directory / f"{seed}.err"

    with out_file.open("w+", encoding="utf-8") as out_fd, \
         err_file.open("w+", encoding="utf-8") as err_fd:
        input = (INPUT_DIRECTORY / f"{seed}.txt").read_text(encoding="utf-8").encode("utf-8")

        try:
            tester_process = subprocess.run([str(TESTER_BINARY), str(solver)], input=input, stdout=out_fd, stderr=err_fd, timeout=5)
        except subprocess.TimeoutExpired:
            raise RuntimeError(f"Tester timed out for seed {seed}")

        if tester_process.returncode != 0:
            raise RuntimeError(f"Tester exited with exit code {tester_process.returncode} for seed {seed}")

    return get_score(err_file)

def run(solver: Path, seeds: List[int], solver_output_directory: Path) -> None:
    if not solver_output_directory.is_dir():
        solver_output_directory.mkdir(parents=True)

    with multiprocessing.Pool(multiprocessing.cpu_count() - 2) as pool:
        pool.map(generate_seed, seeds)

        scores = pool.starmap(run_seed, [(solver, seed, solver_output_directory) for seed in seeds])

        for seed, score in zip(seeds, scores):
            print(f"{seed}: {score:,.0f}")

        if len(seeds) > 0:
            print(f"Total score: {sum(scores):,.0f}")

def main() -> None:
    parser = ArgumentParser(description="Run a solver.")
    parser.add_argument("solver", type=str, help="the solver to run")
    parser.add_argument("--seed", type=int, help="the seed to run (defaults to 0-99)")

    args = parser.parse_args()

    solver = SOLVER_BINARY_ROOT / args.solver
    if not solver.is_file():
        raise RuntimeError(f"Solver not found, {solver} is not a file")

    solver_output_directory = OUTPUT_DIRECTORY / args.solver

    if args.seed is None:
        run(solver, list(range(100)), solver_output_directory)
    else:
        run(solver, [args.seed], solver_output_directory)

    update_overview()

if __name__ == "__main__":
    main()
