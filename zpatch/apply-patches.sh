#!/bin/bash

# folder containing patch files
PATCH_DIR=$1

# target git repo in another folder
TARGET_REPO=$2

# ensure target is a git repo
if [ ! -d "$TARGET_REPO/.git" ]; then
	printf "ERROR: %s is not a Git repository\n" "$TARGET_REPO"
	exit 1
fi

# change to the target repo directory
cd "$TARGET_REPO" || exit 1

shopt -s nullglob

for patch in "$PATCH_DIR"/*.patch; do
		printf "Processing: %s\n" "$patch"

		# Extract the full subject line, including wrapped lines
		subject=$(awk '
				/^Subject:/ {
						sub(/^Subject: /, "")
						subject = $0
						next
				}
				subject && /^[[:space:]]+/ {
						# continuation line
						sub(/^[[:space:]]+/, "")
						subject = subject " " $0
						next
				}
				subject { print subject; exit }
		' "$patch")

		# normalize: strip [PATCH ...] and trailing dot
		clean_subject=$(echo "$subject" | sed -E 's/^\[[^]]+\] *//' | sed 's/\.*$//')

		# check against normalized git log subjects
		already_applied=false
		while IFS= read -r line; do
				normalized_line="${line%%.}"
				if [ "$normalized_line" == "$clean_subject" ]; then
						already_applied=true
						break
				fi
		done < <(git log --format=%s -n 100)

		if $already_applied; then
				printf "  → Skipped: Already applied (%s)\n" "$clean_subject"
		else
				printf "  → Applying patch...\n"
				if ! git am "$patch"; then
						printf "    Failed to apply patch: %s\n" "$patch"
						git am --abort
						exit 1
				fi
		fi
done
