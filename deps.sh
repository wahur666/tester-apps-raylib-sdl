#!/usr/bin/env sh
set -eu

command_name="${1:-install}"
force="false"

if [ "$command_name" = "--force" ] || [ "$command_name" = "-f" ]; then
    command_name="install"
    force="true"
elif [ "${2:-}" = "--force" ] || [ "${2:-}" = "-f" ]; then
    force="true"
fi

case "$command_name" in
    install|status|clean)
        ;;
    *)
        echo "Usage: ./deps.sh [install|status|clean] [--force]" >&2
        exit 2
        ;;
esac

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

deps_names="raylib SDL"
deps_repositories="https://github.com/raysan5/raylib.git https://github.com/libsdl-org/SDL.git"
deps_tags="6.0 release-3.4.10"
deps_directories="raylib-6.0 SDL-release-3.4.10"

require_git() {
    if ! command -v git >/dev/null 2>&1; then
        echo "Could not find git. Install git or add it to PATH." >&2
        exit 1
    fi
}

assert_in_project() {
    target_path=$1
    target_parent=$(dirname -- "$target_path")
    target_name=$(basename -- "$target_path")

    mkdir -p -- "$target_parent"
    resolved_parent=$(CDPATH= cd -- "$target_parent" && pwd)
    full_path="$resolved_parent/$target_name"

    case "$full_path" in
        "$script_dir"/*)
            printf '%s\n' "$full_path"
            ;;
        *)
            echo "Refusing to operate outside the project directory: $full_path" >&2
            exit 1
            ;;
    esac
}

dep_field() {
    index=$1
    shift
    set -- $*
    eval "printf '%s\n' \"\${$index}\""
}

dep_count=2

show_status() {
    i=1
    while [ "$i" -le "$dep_count" ]; do
        directory=$(dep_field "$i" "$deps_directories")
        if [ -d "$script_dir/$directory" ]; then
            echo "$directory: installed"
        else
            echo "$directory: missing"
        fi
        i=$((i + 1))
    done
}

install_deps() {
    require_git

    temp_parent="$script_dir/.deps-tmp"
    mkdir -p -- "$temp_parent"

    i=1
    while [ "$i" -le "$dep_count" ]; do
        name=$(dep_field "$i" "$deps_names")
        repository=$(dep_field "$i" "$deps_repositories")
        tag=$(dep_field "$i" "$deps_tags")
        directory=$(dep_field "$i" "$deps_directories")
        target=$(assert_in_project "$script_dir/$directory")

        if [ -e "$target" ]; then
            if [ "$force" != "true" ]; then
                echo "$directory already exists. Use --force to replace it."
                i=$((i + 1))
                continue
            fi

            rm -rf -- "$target"
        fi

        temp_directory="$temp_parent/$directory-$$-$i"
        rm -rf -- "$temp_directory"

        echo "Installing $name $tag into $directory..."
        if git clone \
            --depth 1 \
            --branch "$tag" \
            --recurse-submodules \
            --shallow-submodules \
            "$repository" \
            "$temp_directory"; then
            mv -- "$temp_directory" "$target"
            echo "Installed $directory."
        else
            rm -rf -- "$temp_directory"
            echo "git clone failed for $name." >&2
            exit 1
        fi

        i=$((i + 1))
    done

    rmdir -- "$temp_parent" 2>/dev/null || true
}

clean_deps() {
    i=1
    while [ "$i" -le "$dep_count" ]; do
        directory=$(dep_field "$i" "$deps_directories")
        target=$(assert_in_project "$script_dir/$directory")

        if [ -e "$target" ]; then
            rm -rf -- "$target"
            echo "Removed $directory."
        fi

        i=$((i + 1))
    done
}

case "$command_name" in
    install)
        install_deps
        ;;
    status)
        show_status
        ;;
    clean)
        clean_deps
        ;;
esac
