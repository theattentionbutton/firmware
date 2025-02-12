# The Attention Button firmware

This repository contains the firmware source code for the Attention Button
device. It uses PlatformIO for managing libraries and the ESP32 build
environment.

## Building

```sh
cd captive-portal-page
pnpm i
cd ..
make
```

## TODO:

- add a silent mode
- add brightness control for display
- add client certificate support (see BearSSL setClientRSACert /
  setClientECCert)
