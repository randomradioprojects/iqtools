# iqtools
various tools to make working with IQ files just a little easier. _At the moment there is only a single tool tho..._

## tool list

### iqtools_wavheader
Simple tool to add headers to raw IQ files to play them back in SDR software easily. Supports u8, s16 and f32

### iqtools_convert
tool to convert wav files (u8, s16 and f32) and raw formats (u8, s8, s16 and f32)

# how do i build/install this?

to install simply
```
mkdir build && cd build
cmake ..
make
sudo make install # if you want to
```

to uninstall:
```
sudo make uninstall
```
