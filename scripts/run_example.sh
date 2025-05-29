set -e

cd "$(dirname $0)/.."

BINARY="build/test"
EXAMPLE="$1"
CC=clang

$BINARY "examples/$EXAMPLE.lawn" "${@:2}"
mv "./out.ll" "examples/$EXAMPLE.ll"

clang -o "examples/$EXAMPLE.out" "$CFLAGS" "examples/$EXAMPLE.ll" "examples/$EXAMPLE.c"

exec "examples/$EXAMPLE.out"
