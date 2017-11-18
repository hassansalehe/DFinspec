#### GRAPH MODELS ====
#         int num_states = 36;
#        int num_observations = 18;
#        int seq_lenght = 3;
HERE=`pwd`
HOME=$HERE/..
BENCHS=$HOME/benchmarks/dwarfs

GRAPH_HOME=$BENCHS/graph_models

# run graph and models
#go to graph
cd $GRAPH_HOME
rm *HBlog*txt Trace*txt
./graph_models_adf -f $HERE/graph_models_input.txt -n 3
# run analysis
$HOME/ADFinspec Trace*.txt HBlog*txt graph_models_adf.cpp.iir

exit

BRANCH_HOME=$BENCHS/branch_bound

# run branch and bound
#go to branch
cd $BRANCH_HOME
rm *HBlog*txt Trace*txt
./branch_bound_adf -f $HERE/branch_bound_input.txt -n 3
# run analysis
$HOME/ADFinspec Trace*.txt HBlog*txt branch_bound_adf.cpp.iir

