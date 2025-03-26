# Capture the user's current directory
ORIGINAL_DIR=$(pwd)

# Get the directory of the current script
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd "$DIR"
odin run ./ -out:lmalloc -o:none -debug -show-timings

cd "$ORIGINAL_DIR"
