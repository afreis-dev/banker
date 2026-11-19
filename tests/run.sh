#!/bin/sh

root=$(cd "$(dirname "$0")/.." && pwd)
banker="$root/banker"
fixtures="$root/tests/fixtures"
work=$(mktemp -d) || { echo "cannot create a temporary directory"; exit 1; }
trap 'rm -rf "$work"' EXIT
trap 'exit 1' INT TERM

if [ ! -x "$banker" ]; then
    echo "banker binary not found, run make first"
    exit 1
fi

passed=0
failed=0

for dir in "$fixtures"/*/; do
    name=$(basename "$dir")
    run="$work/$name"
    mkdir -p "$run"
    cp -R "$dir." "$run/"
    args=$(cat "$run/args")

    (cd "$run" && "$banker" $args >stdout.txt 2>stderr.txt)
    status=$?

    if [ -f "$run/expected.txt" ]; then
        if [ "$status" -eq 0 ] && cmp -s "$run/expected.txt" "$run/result.txt"; then
            result=pass
        else
            result=fail
        fi
    else
        message=$(cat "$run/error")
        if [ "$status" -eq 1 ] && [ ! -f "$run/result.txt" ] && grep -qF -- "$message" "$run/stderr.txt"; then
            result=pass
        else
            result=fail
        fi
    fi

    if [ "$result" = pass ]; then
        passed=$((passed + 1))
        echo "PASS $name"
    else
        failed=$((failed + 1))
        echo "FAIL $name (exit status $status)"
        sed 's/^/    stderr: /' "$run/stderr.txt"
        if [ -f "$run/expected.txt" ] && [ -f "$run/result.txt" ]; then
            diff "$run/expected.txt" "$run/result.txt" | sed 's/^/    diff: /'
        fi
    fi
done

echo "$passed passed, $failed failed"
[ "$failed" -eq 0 ]
