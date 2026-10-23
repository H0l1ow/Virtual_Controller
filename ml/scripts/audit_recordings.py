"""Entry point for the M6.4 recording-quality preflight."""
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from vcml.audit import main

if __name__ == "__main__":
    raise SystemExit(main())
