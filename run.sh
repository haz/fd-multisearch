
#./fast-downward.py --translate dom.pddl prob.pddl --translate-options --keep-unimportant-variables
#./fast-downward.py --debug multi-output.sas --search "multiple(lazy_greedy(ff()))"
#./fast-downward.py --debug multi-output.sas --heuristic "h=ff(transform=adapt_costs(cost_type=ONE))" --search "multiple(lazy_greedy(h,preferred=[h]))"

#BUILD=debug32
BUILD=release64

TARGET=multi-output-small.sas
#TARGET=multi-output-big.sas

if [[ $# -eq 0 ]] ; then
    ./fast-downward.py --build="$BUILD" "$TARGET" --search "multiple()"
elif [[ "$1" == "build" ]] ; then
    ./build.py -j 6 "$BUILD"
elif [[ "$1" == "memleak" ]] ; then
    valgrind --tool=memcheck --leak-check=yes ./builds/$BUILD/bin/downward --search "multiple()" --internal-plan-file sas_plan < "$TARGET" > output.log 2>&1
    gedit output.log
    rm output.log
elif [[ "$1" == "memuse" ]] ; then
    valgrind --tool=massif --stacks=yes --massif-out-file=massif.log ./builds/$BUILD/bin/downward --search "multiple()" --internal-plan-file sas_plan < "$TARGET" > /dev/null 2>&1
    ms_print massif.log
    rm -rf massif.log
elif [[ "$1" == "profile" ]] ; then
    valgrind --tool=callgrind --dump-instr=yes --collect-jumps=yes ./builds/$BUILD/bin/downward --search "multiple()" --internal-plan-file sas_plan < "$TARGET" > /dev/null
    kcachegrind callgrind.out.*
    #python gprof2dot.py --format=callgrind --output=out.dot callgrind.out.*; dot -T png out.dot -o out.png
    rm -rf callgrind.out.*
    rm -rf out.dot
fi

rm -rf sas_plan.*
