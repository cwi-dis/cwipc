#!/bin/bash
#
# Glue for the install-cwipc GitHub Action (action.yml): run get-cwipc.sh, then
# pass the environment from <prefix>/libexec/cwipc/environment on to the
# following steps of the job through GITHUB_PATH and GITHUB_ENV.
# All the real work is in scripts/get-cwipc.sh and scripts/setup-cwipc.sh,
# which are usable without GitHub too. See cwi-dis/cwipc#353.
#
set -euo pipefail

windows=false
case "$(uname -s)" in
MINGW*|MSYS*|CYGWIN*) windows=true ;;
esac
unixpath() {
    if $windows; then cygpath -u "$1"; else echo "$1"; fi
}

# The action is downloaded with the whole repository, so the scripts are here:
scripts="$GITHUB_ACTION_PATH/../../../scripts"

args=()
[ -n "$INPUT_METHOD" ] && args+=(--method "$INPUT_METHOD")
[ -n "$INPUT_ARCHIVE" ] && args+=(--archive "$(unixpath "$INPUT_ARCHIVE")")
[ "$INPUT_DEPS" = true ] && args+=(--deps)
[ "$INPUT_PYTHON_MODULES" = true ] || args+=(--no-python)
[ -n "$INPUT_PYTHON" ] && args+=(--python "$INPUT_PYTHON")
prefix="$INPUT_PREFIX"
[ -n "$prefix" ] || prefix="$RUNNER_TEMP_DIR/cwipc"
prefix=$(unixpath "$prefix")

prefix_file=$(mktemp)
bash "$scripts/get-cwipc.sh" "${args[@]}" --prefix-file "$prefix_file" "$INPUT_VERSION" "$prefix"
prefix=$(cat "$prefix_file")
rm -f "$prefix_file"

envfile="$prefix/libexec/cwipc/environment"
[ -f "$envfile" ] || { echo "install-cwipc: no environment file $envfile" >&2; exit 1; }
echo "install-cwipc: environment from $envfile:"
venvpython=""
while read -r kind rest; do
    case "$kind" in
    ''|'#'*) continue ;;
    path)
        echo "$rest" >> "$GITHUB_PATH"
        # GITHUB_PATH only takes effect in the next step; we need it for the version check below
        # (on Windows cwipc's Python finds cwipc_util.dll through PATH only).
        export PATH="$(unixpath "$rest"):$PATH"
        ;;
    env)
        echo "$rest" >> "$GITHUB_ENV"
        name="${rest%%=*}"
        export "$name=${rest#*=}"
        ;;
    libpath)
        name="${rest%% *}"
        dir="${rest#* }"
        current="${!name:-}"
        echo "$name=$dir${current:+:$current}" >> "$GITHUB_ENV"
        export "$name=$dir${current:+:$current}"
        ;;
    *)
        echo "install-cwipc: unknown line in $envfile: $kind $rest" >&2
        exit 1
        ;;
    esac
    echo "  $kind $rest"
done < "$envfile"

version=""
if [ -n "${VIRTUAL_ENV:-}" ]; then
    venv=$(unixpath "$VIRTUAL_ENV")
    if $windows; then venvpython="$venv/Scripts/python.exe"; else venvpython="$venv/bin/python"; fi
    version=$("$venvpython" -c 'import cwipc; print(cwipc.cwipc_get_version())')
    echo "install-cwipc: cwipc version $version"
fi

if $windows; then
    prefix=$(cygpath -w "$prefix")
fi
{
    echo "prefix=$prefix"
    echo "cwipc-version=$version"
} >> "$GITHUB_OUTPUT"
