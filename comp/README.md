| STEP_A_DAY | slow.c | fast.c |
| --- | --- | --- |
| 288 | 61.53s | 26.50s |
| 2880 | 584.91s | 259.82s |

```
gcc -O3 -march=native -mtune=native -flto -funroll-loops -o slow ./slow.c -lcrypto
gcc -O3 -march=native -mtune=native -flto -funroll-loops -o fast ./fast.c -lcrypto
```