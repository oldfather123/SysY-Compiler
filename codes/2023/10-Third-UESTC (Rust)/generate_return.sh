#/bin/bash

source_dir=./test/functional/
target_dir=./test/functional_return/

if [ ! -d $source_dir ]; then
    echo "Source directory $source_dir does not exist"
    exit 1
fi

if [ ! -d $target_dir ]; then
    mkdir -p $target_dir
fi

for file in `ls $source_dir/*.sy`; do
  base=$(basename $file)
  echo "$source_dir/$base -> $target_dir/${base%.sy}.c"
  cat $source_dir/$base > $target_dir/${base%.sy}.c
  gcc -o $target_dir/${base%.sy} $target_dir/${base%.sy}.c ./test/functional_return/sylib.c
  if [ -e ./test/stdin/$base ]; then
    $target_dir/${base%.sy} < ./test/stdin/$base
  else
    $target_dir/${base%.sy} < /dev/null
  fi
  echo $? > $target_dir/${base%.sy}.res
  #rm -rf $target_dir/${base%.sy}.c $target_dir/${base%.sy}
done
