### Export enviroment variable

```
export SPSFAST_PATH=/path/to/sps/root/
```

### Model data

- binary models on this link: https://drive.google.com/open?id=0B1mTQzI2DYe-UmdMUXJLbGxnTTA


### Compile all

```
make all
```



### generating spectra

- compile:

```
make bin/generate_spec
```

- run

```
cd bin
./generate_spec test.cfg test_params.tsv ../output/results.tsv
```

### generating spectra only on cpu

- compile:

```
make bin/generate_spec_cpu
```

- run

```
cd bin
./generate_spec_cpu test.cfg test_params.tsv ../output/results.tsv
```

