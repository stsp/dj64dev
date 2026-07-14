#!/bin/sh

set -e -x

make demos_static
make -C demos run
make demos_clean demos
make -C demos run
make -C demos runelf
cd demos/hello
ls -l hello
make host.elf
RESULT=`DJ64_DUMB_VIDEO=1 ./host.elf | dos2unix`
if [ "$RESULT" != "Hello World!" ]; then
  echo "bad test output: $RESULT"
  echo "Test FAILED"
  exit 1
fi
echo "Test PASSED"
