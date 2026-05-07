#!bin/bash

set -e

if [ $# -gt 0 ]; then
	pathorfile=$1
	if test -d $pathorfile; then
		path=$pathorfile # is path
		for file in $path/*.sy; do
			mv $file $file.tmp
		done
	else
		file=$pathorfile # is file
		mv $file $file.tmp
	fi
else
	echo "use it like: sh scripts/syfile2tmp.sh ./test/functional/52_scope.sy \
or $: sh scripts/syfile2tmp.sh test/functional"
fi
