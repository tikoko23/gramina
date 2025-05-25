set -e

cd "$(dirname $0)/.."

BINARY="build/test"
EXAMPLE="$1"
CC=clang

$BINARY "examples/$EXAMPLE.lawn"
mv "./out.ll" "examples/$EXAMPLE.ll"

clang -o "examples/$EXAMPLE.out" "examples/$EXAMPLE.ll" "examples/$EXAMPLE.c"

exec "examples/$EXAMPLE.out"
