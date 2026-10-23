# Virtual Controller M6.4 - recording audit, causal TCN training and evaluation

The Qt/C++ application already supports **optional ONNX inference**. M6.0
connects the M5 labeled JSONL recordings to training, offline validation, a
held-out Rules comparison, and optional deployment. **No production trained
model is bundled**: it is not possible to measure real-world accuracy without
suitable recordings of real hands.

## Native model contract (unchanged)

| Property | Value |
| --- | --- |
| Input features | `vc.hand134.v2` / float32 `[batch, 16, 134]` |
| Sampling | 30 Hz, causal zero-order hold, never use a future frame |
| Output logits | `[batch, 5]` |
| Labels | `NONE, FIST, OPEN_HAND, POINT, PINCH` |
| Normalization | Derived from train split only; embedded in ONNX |
| Classifier | causal TCN, dilation 1/2/4 |
| Inference | ONNX Runtime C++, `VC_WITH_ONNX=ON` (opt-in) |

**Dynamic SWIPE labels are not implemented in this schema.** `hand134.v2`
subtracts the wrist and does not preserve translation of the entire hand;
dynamic gestures require a new feature/model contract.

## 1. Record a real dataset with the current application

- `ML > Dataset recording`: select the correct label for LEFT
  and RIGHT; use `UNLABELED` for hands you are *not* supervising. Use `NONE`
  for a tracked hand with no intentional gesture.
- Keep one ground-truth gesture stable throughout each recorded session;
  mislabeled transition frames will contaminate the dataset. Collect separate
  sessions for all five classes and both hands. Include varied lighting,
  distance, angles, individuals and intentional no-gesture samples.
- Windows appear after 16 valid 30-Hz samples (roughly 0.5 seconds), so
  very short recordings yield no training windows.
- Collect **at least three independent participants** for subject-held-out
  tests, ideally substantially more. With one participant, only session-held-out
  evaluation is possible; it cannot estimate generalization to new users.
- Files contain 21 hand landmarks (not images); treat them as personal data.
  Do not commit/share recordings without participants' consent.

Sessions are under the Windows per-user AppLocalDataLocation `gesture_sessions`
folder. The path of each recording is displayed on the ML page. Copy
sessions into one folder without renaming them to the same name.

## 2. Install the offline Python environment

```powershell
py -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r ml\requirements.txt
```

The normal Qt Creator Build/Run/Debug still **does not need Python**.

## 3. Audit recording quality **before** building a dataset (M6.4)

The new preflight tool reads the same JSONL and uses the same 30-Hz / 16-frame
feature contract as `prepare_dataset.py`. It checks schema, temporal gaps,
track resets, geometrically invalid samples, trainable windows per class and
hand, and coverage per split group. Reports contain **counts, filenames and
warnings, not raw landmarks**. The audit neither verifies manual labels nor
makes claims about real-world accuracy.

For the audit alone, only NumPy is required (not PyTorch or ONNX):

```powershell
py -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r ml\requirements-audit.txt
```

Example without subject manifest (session-safe only, *not* person-safe):

```powershell
.\.venv\Scripts\python.exe ml\scripts\audit_recordings.py --input data\recordings --out data\reports\recording_audit.json
```

If several recordings belong to the same person, create a CSV `file,subject_id`
covering **every** JSONL recording, then use:

```powershell
.\.venv\Scripts\python.exe ml\scripts\audit_recordings.py --input data\recordings --subjects data\subjects.csv --out data\reports\recording_audit.json
```

Return codes are `0` for basic dataset-build readiness, `2` for incomplete
coverage or invalid recordings, and `1` for invalid arguments/IO/manifest.
The JSON also reports stricter `readyForFiveClassHeldOutEvaluation`.

**Interpretation:** At least three independent people with all five gesture
classes are recommended for a subject-held-out train/validation/test evaluation.
With one person, you can still inspect session-level behavior, but do not claim
generalization to unseen users. The default **100 windows/class** is a *low
volume warning threshold*, not proof that training data are sufficient:
overlapping windows are highly correlated. Prefer multiple sessions, positions,
light conditions and participants for every gesture and both hands.

The report never determines whether an intentional `FIST` was accidentally
labeled `OPEN_HAND`. Review a subset of recordings manually and keep a stable
pose during each labeled session. Never treat the preflight status as a
safety certification for game, keyboard or mouse output.

## 4. Build the dataset (no leakage across groups)

```powershell
.\.venv\Scripts\python.exe ml\scripts\prepare_dataset.py --input data\recordings --out data\processed\dataset.npz --stride 2 --seed 42
```

Use a manifest if one person has multiple recording sessions, e.g.:

```csv
file,subject_id
gesture_session_001.jsonl,user_A
gesture_session_002.jsonl,user_A
gesture_session_003.jsonl,user_B
```

Then append `--subjects data\subjects.csv`. **Every recording** must be
present in the manifest. File names may be relative paths under the input
directory. If omitted, each recording is treated as a separate person/session;
that prevents *session* leakage, but not *person* leakage. The builder needs at
least 3 distinct split groups for train/validation/test. All overlapping windows
from the same group stay in the same split. Windowing resets on changed label,
lost tracking, changed identity and gaps >150 ms.

Output: `dataset.npz` and `dataset.json`. Inspect `classCounts` and
`missingClassesPerSplit` in the JSON. The train split must include all five
classes; collect more samples if any are absent. The split algorithm is
approximately group-stratified and deterministic with the chosen seed.

NPZ fields include `X`, `y`, `split`, `session`, `subject`, `side`, `segment`,
`timestamp_us`, `rule_scores`. `split`: `0=train, 1=validation, 2=test`.

## 5. Train TCN (PyTorch)

```powershell
.\.venv\Scripts\python.exe ml\scripts\train.py --data data\processed\dataset.npz --run runs\tcn_01 --epochs 60 --patience 10 --device cpu
```

Training uses only the training split for normalization and gradient updates.
Validation drives checkpoint selection/early stopping. **Test is never used
for training or tuning**. Inverse-square-root class weights reduce imbalance.
`best.pt` contains the checkpoint and validation score. `config.json` records
the dataset SHA256, model configuration and class counts.

## 6. Evaluate TCN vs Rules (held-out test)

```powershell
.\.venv\Scripts\python.exe ml\scripts\evaluate.py --data data\processed\dataset.npz --run runs\tcn_01 --out runs\tcn_01\test_report.json
```

Output includes a confusion matrix, per-class precision/recall/F1, macro-F1,
`NONE` false-positive rate and an *approximate* event activation count/latency.
The same split/windows are used for the offline Rules baseline. The Rules
values are geometric heuristics, not probabilities. Event metrics do **not**
reproduce every GestureStateManager parameter or SendInput; only a real camera
and end-to-end test can establish real false activations and latency.

Review subject and class coverage before interpreting metrics. Overlapping
windows are correlated; comparing their F1 does not establish statistically
independent results. For model selection, compare validation metrics first and
only use the test split once for final reporting.

## 7. Export ONNX and deploy on Windows

```powershell
.\.venv\Scripts\python.exe ml\scripts\export_onnx.py --run runs\tcn_01 --out models --evaluation runs\tcn_01\test_report.json
```

Produces `models/gesture_model.onnx`, `model_metadata.json` and
`parity_report.json`. The exporter checks logits parity against Python ONNX
Runtime when installed. The native loader requires a matching metadata SHA256
so an ONNX file cannot silently be swapped without updating metadata.

In Qt Creator:

1. Build with MSVC 2022 x64 and Qt 6.11.2.
2. Set `VC_WITH_ONNX=ON` and `VC_ONNXRUNTIME_ROOT=C:/SDK/onnxruntime-win-x64-1.26.0`
   in CMake configuration; install SDK first.
3. Reconfigure + **Build** after exporting; the model and metadata are copied
   to the executable's `models/` directory. When the app is already running,
   `Reload model` rereads the current deployed file but does not perform an
   export or an SDK install.
4. `ML / Model` displays `ONNX TCN` on success; invalid or absent model
   falls back to Rules. System output remains disarmed after model reload.
5. Validate basic gestures in `Gestures` and `Test` **without arming system
   input**; enable it deliberately only after evaluating false activations.

No gamepad backend, new dynamic gestures or model training within QML are
included in M6.0. Those are separate follow-up projects.

## Verification

```powershell
.\.venv\Scripts\python.exe -m unittest discover -s ml\tests -p "test_*.py"
```

The tests use generated geometry **only as software test fixtures** and are
not claimed to train or validate a real-world gesture model. The exported
ONNX model must be evaluated with real recordings before deployment.
