#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_DIR"

make >/dev/null

rm -f sample.txt
rm -rf testdir

OUTPUT_FILE="/tmp/customsh_test_output.txt"
cat <<'EOF' | ./customsh > "$OUTPUT_FILE"
ground
forge sample.txt
inscribe sample.txt direct os shell
peek sample.txt
etch sample.txt second line
peek sample.txt
peek sample.txt > copied.txt
peek copied.txt
nest testdir
sweep
peek sample.txt | summon wc -l
summon /bin/pwd
vanish sample.txt
vanish copied.txt
quit
EOF

echo "Shell test output:"
cat "$OUTPUT_FILE"

echo
echo "Smoke test completed."
