#/bin/bash

source_dir=./test/functional/
target_dir=./test/functional_return/

for file in `ls $source_dir/*.sy`; do
  base=$(basename $file)
  echo "test $base"
  sh ./test.sh $base
done
