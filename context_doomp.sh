#!/bin/bash

set -e

current_datetime=$(date "+%Y-%m-%d_%H-%M-%S")
output_file="xfce4-newtonmenu-dump-${current_datetime}.txt"

base_dir="."
target_dirs=("panel-plugin")
include_files=("meson.build" "meson_options.txt" "*.desktop.in")
include_ext=("*.c" "*.h" "*.vala" "*.ui" "*.xml")

echo "Context dump for XFCE Newton Menu plugin" > "$output_file"
echo "Generated at $current_datetime" >> "$output_file"
echo >> "$output_file"

# Dump tree for full structure (without file dump)
echo "Full directory tree:" >> "$output_file"
tree "$base_dir" >> "$output_file"
echo >> "$output_file"

# Dump selected files
echo "Selected build files and plugin sources:" >> "$output_file"

dump_file() {
    local file="$1"
    echo "File: $file" >> "$output_file"
    echo '```' >> "$output_file"
    cat "$file" >> "$output_file"
    echo '```' >> "$output_file"
    echo >> "$output_file"
}

# Dump meson.build, .desktop.in files from any dir
find "$base_dir" -type f \( -name "meson.build" -o -name "meson_options.txt" -o -name "*.desktop.in" \) | while read -r file; do
    dump_file "$file"
done

# Dump plugin directory contents
for dir in "${target_dirs[@]}"; do
    find "$dir" -type f \( $(printf -- '-name "%s" -o ' "${include_ext[@]}" | sed 's/ -o $//') \) | while read -r file; do
        dump_file "$file"
    done
done

echo "✅ Dump saved to $output_file"

