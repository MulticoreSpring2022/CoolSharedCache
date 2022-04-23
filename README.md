# Multicore Project

---

## Introduction

Our team's final project structure is based on [ChampSim](https://github.com/ChampSim/ChampSim). What our team contributed is located at the cache replacement policy that is located at /replacement. It contains four algorithms, EAF, LFRU, PLRU, LRU-K, you can switch to run each of them.

---

## Environment

Our project should be run on **crunchy1** machine on CIMS. Please be sure that your are at that machine. Then you should use `gcc-9.2` to compile the program.

``` sh
$ module load gcc-9.2
```

---

## Download Trace

Due to limited volume in the CIMS machine, our team does not place our running data in the project. However, we did place a script for you to download two of them and run a two cores example. (Be sure to add the permission the script)

``` shell
$ ./download_data.sh 
```

This would download `401.bzip2-226B.champsimtrace.xz` and  `401.bzip2-277B.champsimtrace.xz`. If you want ot test the 435 and 437, you should download them via [Traces](https://hpca23.cse.tamu.edu/champsim-traces/speccpu/index.html).

---

## Compile

ChampSim takes a JSON configuration script. Examine `champsim_config.json` for a fully-specified example. All options described in this file are optional and will be replaced with defaults if not specified. The configuration scrip can also be run without input, in which case an empty file is assumed.

```shell
$ ./config.sh champsim_config.json
$ make
```

---

# Run simulation

Execute the binary directly (Example to run a two core simulator).

```shell
$ bin/champsim --warmup_instructions 10000000 --simulation_instructions 50000000 traces/401.bzip2-226B.champsimtrace.xz traces/401.bzip2-277B.champsimtrace.xz
```

The number of warmup and simulation instructions given will be the number of instructions retired. Note that the statistics printed at the end of the simulation include only the simulation phase.

---

## Run Different Algorithms

You can modify the `champsim_config.json` with the following content with the directory name in /replacement ("eaf", "lru", "plru", "lfru", "lru-k").

``` json
    "LLC": {
        "frequency": 4000,
        "sets": 2048,
        "ways": 16,
        "rq_size": 32,
        "wq_size": 32,
        "pq_size": 32,
        "mshr_size": 64,
        "latency": 20,
        "max_read": 1,
        "max_write": 1,
        "prefetch_as_load": false,
        "virtual_prefetch": false,
        "prefetch_activate": "LOAD,PREFETCH",
        "prefetcher": "no",
        "replacement": "eaf" // Modify here, eample "lfru"
    },
```

Then, recompile and run the simulator again.

---

### Run Different Cores

If you want to run 4 cores, you should modify the `num_cores` in `champsim_config.json` and copy the content in `ooo_cpu` to the corresponding number.

``` json
    "num_cores": 2, // modify here

    "ooo_cpu": [
        {
            // ...
        },
        {
            // ...
        }
        // add more {} to the num_cores.
    ],

    "DIB": {
        "window_size": 16,
        "sets": 32,
        "ways": 8
    },
```

Then recompile and run the simulaotr again.

---

