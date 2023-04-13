import hcr
from pathlib import Path

PROJECT_ROOT = Path(__file__).parent.parent

solvers = sorted([file.stem for file in (PROJECT_ROOT / "cmake-build-release").iterdir() if file.is_file() and file.name.startswith("v")])
default_seeds = list(range(100))
is_maximizing = False
results_directory = PROJECT_ROOT / "results"
visualizer_link_template = "https://img.atcoder.jp/ahc018/6bada50282.html?lang=en&seed=$SEED$&solver=$SOLVER$"

def get_input(seed: str) -> str:
    with hcr.temporary_file(seed + "\n") as seed_file, hcr.temporary_directory() as output_directory:
        hcr.run_process([PROJECT_ROOT / "scripts" / "gen", seed_file], cwd=output_directory)
        return (output_directory / "in" / "0000.txt").read_text(encoding="utf-8")

def run_solver(solver: str, input: str) -> hcr.Output:
    return hcr.run_process([PROJECT_ROOT / "scripts" / "tester", PROJECT_ROOT / "cmake-build-release" / solver], input=input, timeout=5)

def get_score(output: hcr.Output) -> float:
    total_cost_line = next((line for line in output.stderr.splitlines() if line.startswith("Total Cost = ")), None)
    return int(total_cost_line.split(" = ")[1]) if total_cost_line is not None else 0

if __name__ == "__main__":
    hcr.cli(solvers,
            default_seeds,
            is_maximizing,
            results_directory,
            get_score,
            visualizer_link_template=visualizer_link_template,
            get_input=get_input,
            run_solver=run_solver)
