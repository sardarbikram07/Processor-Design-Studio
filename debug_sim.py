import subprocess
import json

# Quick functional correctness check for loop_sum with forwarding
config = {
    "benchmark": "loop_sum",
    "pipeline": {"enabled": True, "forwarding": True, "stalling": True},
    "cache": {"sizeKB": 0, "blockSize": 64, "associativity": 1, "replacement": "LRU", "writePolicy": "write-through"},
    "branchPredictor": {"type": "STATIC_NOT_TAKEN"}
}

result = subprocess.run(
    [r"sim-core\build\sim.exe", "--benchmarks-dir", "sim-core/benchmarks"],
    input=json.dumps(config),
    capture_output=True, text=True, cwd=r"D:\Desktop\Processor design studio"
)
print("STDOUT:", result.stdout)
print("STDERR:", result.stderr)
