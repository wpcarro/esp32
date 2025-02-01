# Forth on ESP32

I found this workflow to be useful.

```shell
$ # Run this in one terminal
$ echo main.fth upload.sh | tr ' ' '\n' | entr ./upload.sh
```

```shell
$ # Run this in another terminal
$ echo Forth.ino flash.sh | tr ' ' '\n' | entr ./flash.sh
```
