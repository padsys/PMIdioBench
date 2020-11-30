#!/usr/bin/env bash

readonly BASEDIR=$(readlink -f "$(dirname "$0")")/..
cd "$BASEDIR"

# exit on errors
set -e

rc=0

echo -n "Checking file permissions..."

while read -r perm _res0 _res1 path; do
	if [ ! -f "$path" ]; then
		continue
	fi

	fname=$(basename -- "$path")

	case ${fname##*.} in
		c|h|cpp|cc|cxx|hh|hpp|md|html|js|json|svg|Doxyfile|yml|LICENSE|README|conf|in|Makefile|mk|gitignore|go|txt)
			# These file types should never be executable
			if [ "$perm" -eq 100755 ]; then
				echo "ERROR: $path is marked executable but is a code file."
				rc=1
			fi
		;;
		*)
			shebang=$(head -n 1 "$path" | cut -c1-3)

			# git only tracks the execute bit, so will only ever return 755 or 644 as the permission.
			if [ "$perm" -eq 100755 ]; then
				# If the file has execute permission, it should start with a shebang.
				if [ "$shebang" != "#!/" ]; then
					echo "ERROR: $path is marked executable but does not start with a shebang."
					rc=1
				fi
			else
				# If the file does not have execute permissions, it should not start with a shebang.
				if [ "$shebang" = "#!/" ]; then
					echo "ERROR: $path is not marked executable but starts with a shebang."
					rc=1
				fi
			fi
		;;
	esac

done <<< "$(git grep -I --name-only --untracked -e . | git ls-files -s)"

if [ $rc -eq 0 ]; then
	echo " OK"
fi

if hash astyle 2> /dev/null; then
	echo -n "Checking coding style..."
	rm -f astyle.log
	touch astyle.log
	git ls-files '*.[ch]' '*.cpp' '*.cc' '*.cxx' '*.hh' '*.hpp' | \
		grep -v cpp_headers | \
		xargs astyle --options=.astylerc >> astyle.log
	if grep -q "^Formatted" astyle.log; then
		echo " errors detected"
		git diff
		sed -i -e 's/  / /g' astyle.log
		grep --color=auto "^Formatted.*" astyle.log
		echo "Incorrect code style detected in one or more files."
		echo "The files have been automatically formatted."
		echo "Remember to add the files to your commit."
		rc=1
	else
		echo " OK"
	fi
	rm -f astyle.log
else
	echo "You do not have astyle installed so your code style is not being checked!"
fi

echo -n "Checking comment style..."

git grep --line-number -e '/[*][^ *-]' -- '*.[ch]' > comment.log || true
git grep --line-number -e '[^ ][*]/' -- '*.[ch]' >> comment.log || true
git grep --line-number -e '^[*]' -- '*.[ch]' >> comment.log || true
git grep --line-number -e '//' -- "$(git ls-files '*.[ch]')" >> comment.log || true

if [ -s comment.log ]; then
	echo " Incorrect comment formatting detected"
	cat comment.log
	rc=1
else
	echo " OK"
fi
rm -f comment.log

echo -n "Checking for spaces before tabs..."
git grep --line-number $' \t' -- > whitespace.log 2> /dev/null || true
if [ -s whitespace.log ]; then
	echo " Spaces before tabs detected"
	cat whitespace.log
	rc=1
else
	echo " OK"
fi
rm -f whitespace.log

echo -n "Checking trailing whitespace in output strings..."

git grep --line-number -e ' \\n"' -- '*.[ch]' > whitespace.log 2> /dev/null || true

if [ -s whitespace.log ]; then
	echo " Incorrect trailing whitespace detected"
	cat whitespace.log
	rc=1
else
	echo " OK"
fi
rm -f whitespace.log

echo -n "Checking for use of forbidden library functions..."

git grep --line-number -w '\(strncpy\|strcpy\|sprintf\|vsprintf\)' -- '*.c' > badfunc.log 2> /dev/null || true
if [ -s badfunc.log ]; then
	echo " Forbidden library functions detected"
	cat badfunc.log
	rc=1
else
	echo " OK"
fi
rm -f badfunc.log

echo -n "Checking blank lines at end of file..."

if ! git grep -I -l -e . -z | \
	xargs -0 -P8 -n1 scripts/eofnl > eofnl.log; then
	echo " Incorrect end-of-file formatting detected"
	cat eofnl.log
	rc=1
else
	echo " OK"
fi
rm -f eofnl.log

echo -n "Checking for POSIX includes..."
git grep -I -i -f scripts/posix.txt -- '*.[cc|h]' ':!scripts/posix.txt' ':!include/stdinc.h' > scripts/posix.log 2> /dev/null || true
if [ -s scripts/posix.log ]; then
	echo "POSIX includes detected. Please include stdinc.h instead."
	cat scripts/posix.log
	rc=1
else
	echo " OK"
fi
rm -f scripts/posix.log

exit $rc
