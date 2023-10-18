# Letter count

Count each alphabeth letter in a file.

## Instructions
Install the dependencies:

``` shell
sudo apt install build-essential # For Debian, Ubuntu, ...
```

Build the source code:
``` shell
git clone https://github.com/drcor/letter-count.git
make
```

And finally run:
``` shell
#./letter-count -t <number of threads> -f <file name> -b <size of reading blocks>
./letter-count -t 4 -f <file name> -b 2048
```

