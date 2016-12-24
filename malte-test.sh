#! /bin/bash
BUILD="$1"
if [[ "$BUILD" == "" ]]; then
    BUILD=release64
fi
./build.py -j4 "$BUILD"
time ./fast-downward.py --build="$BUILD" multi-output.sas --search "multiple(lazy_greedy(ff()))" > OUTPUT
wc -l OUTPUT
grep "phases completed" OUTPUT || \
    echo "$(grep -c "Solution found" OUTPUT) tasks solved"

## Some numbers of tasks solved on dakar:
## - original version: 27328
## - remove most output; don't save plans: 66692
##   - same, debug32 build: 58823
##   - same, release64 build: 80046
