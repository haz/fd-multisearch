
#./fast-downward.py --translate dom.pddl prob.pddl --translate-options --keep-unimportant-variables
./build.py --debug -j 6
./fast-downward.py --debug multi-output.sas --search "multiple(lazy_greedy(ff()))"
#./fast-downward.py --debug multi-output.sas --heuristic "h=ff(transform=adapt_costs(cost_type=ONE))" --search "multiple([lazy_greedy(h,preferred=[h])])"

rm sas_plan.*

