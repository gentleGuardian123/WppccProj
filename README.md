# Combination of Multi-party Private Set Intersection && Union

## Intro

Privacy Computing with regard to Smart Affair based on PIR, PSI and MPC.

## Toolchain

| tool          | version                                                 |
| :---          | :----                                                   |
| Linux         | 22.04.1-Ubuntu x86_64 GNU/Linux (ensure **Unix-like**!) |
| gcc/g++       | gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0               |
| make          | GNU Make 4.3 x86_64                                     |
| cmake         | cmake version 3.22.1 (ensure **>= 3.16**)               |
| docker        | Docker version 26.1.3, build b72abbb                    |


## Quickstart

1. Run with the command below (substitute `yourproxy:yourport` with proper proxy which can `access github.com`):
```
$ sudo docker build . -t mpsu_apsi:v1 --network=host --build-arg HTTPS_PROXY=http://yourproxy:yourport --build-arg HTTP_PROXY=http://yourproxy:yourport
```

