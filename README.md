Compile by `make`

Search algorithms:
- `b` Baseline
- `m` MakespanAdd
- `p` PruneAndCut
- `c` Combined

Path finder algorithms:
- `b` Biaset
- `r` TrullyRandom
- `w` WithoutCrossing
- `x` WithoutCrossingAtSameTimes

Run experiments by `./build/mapf_experiments -{b|m|p|c} -{b|r|w|x} {number of threads} {relative path to tests folder}`
Example of runing experiemnts `./build/mapf_experiments -mp -wx 4 Experiments/Tests`

Now time limit for each instance is hardcoded 1m (I am planning to add extra run parameter for changing it by user)

Be aware of high memory requirements while using multiple number of threads
