Here's a simple README you can drop into your project alongside the script:

```markdown
# Bruce Firmware Cloner

A simple Python script to clone the [Bruce firmware](https://github.com/pr3y/Bruce) repository into a directory of your choice.

## Requirements

- Python 3.7+
- Git (optional — use `--zip` if you don't have it)

## Usage

Clone into the current directory:
```bash
python bruce_clone.py
```

Clone into a specific directory:
```bash
python bruce_clone.py -d ~/projects/firmware
```

## Options

| Flag | Description |
|------|-------------|
| `-d`, `--dir` | Target directory (default: current directory) |
| `-b`, `--branch` | Specific branch to clone |
| `--depth` | Shallow clone depth (e.g. `1` for latest only) |
| `--deps` | Install Python build dependencies after cloning |
| `--zip` | Download as ZIP instead of git clone (no git required) |

## Examples

Shallow clone with dependencies:
```bash
python bruce_clone.py -d ~/projects --depth 1 --deps
```

Clone without git:
```bash
python bruce_clone.py -d ~/projects --zip
```

## After Cloning

Navigate into the folder and build with PlatformIO:
```bash
cd Bruce
pio run -e <board_env>
```

Replace `<board_env>` with your hardware target (e.g. `m5stack-cardputer`).

## Notes

- No external Python packages required to run the script.
- If the target `Bruce/` folder exists and isn't empty, you'll be prompted before overwriting.

## License

Script is provided as-is. Bruce firmware is licensed under its own terms — see the [official repo](https://github.com/pr3y/Bruce).
```

Want me to add a section for troubleshooting or a quick "one-liner" install command?