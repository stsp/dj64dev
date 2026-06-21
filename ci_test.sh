#!/bin/sh

set -e -x

make demos -j 9
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
