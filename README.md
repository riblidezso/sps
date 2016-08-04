### Export enviroment variable

```
export SPSFAST_PATH=/path/to/sps/root/
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


### fitting spectra

- compile:

```
make 
```

- run

```
cd bin
./spectrum test.cfg
```


