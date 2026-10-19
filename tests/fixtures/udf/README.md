The compressed UDF volume was generated with Ubuntu's genisoimage 1.1.11:

```
genisoimage -quiet -udf -o base.udf files
```

Its disposable input contains `alpha.bin` (32 repetitions of bytes 0…255),
`empty` (zero bytes), and `nested/tail.bin` (`directory payload`). No user data
is included. Tests mutate copies to exercise allocation kinds and EOF boundaries;
genisoimage is not needed to run the tests.
