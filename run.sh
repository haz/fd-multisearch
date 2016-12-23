
#./fast-downward.py --translate dom.pddl prob.pddl --translate-options --keep-unimportant-variables
#./fast-downward.py --debug multi-output.sas --search "multiple(lazy_greedy(ff()))"
#./fast-downward.py --debug multi-output.sas --heuristic "h=ff(transform=adapt_costs(cost_type=ONE))" --search "multiple(lazy_greedy(h,preferred=[h]))"

if [[ $# -eq 0 ]] ; then
    ./fast-downward.py --debug multi-output.sas --search "multiple(lazy_greedy(ff()))"
elif [[ "$1" == "build" ]] ; then
    ./build.py --debug -j 6
elif [[ "$1" == "memory" ]] ; then
    valgrind --leak-check=yes ./builds/debug32/bin/downward --search "multiple(lazy_greedy(ff()))" --internal-plan-file sas_plan < multi-output.sas > output.log 2>&1
    gedit output.log
    rm output.log
elif [[ "$1" == "profile" ]] ; then
    valgrind --tool=callgrind --dump-instr=yes --collect-jumps=yes ./builds/debug32/bin/downward --search "multiple(lazy_greedy(ff()))" --internal-plan-file sas_plan < multi-output.sas
    kcachegrind callgrind.out.*
    rm -rf callgrind.out.*
fi

rm -rf sas_plan.*
