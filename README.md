# auto-ryzenadj
A c++ application that automatically applies custom ryzenadj profiles

# Installation 
## Step 1: generating build files
### With Applet
```sh
cmake . -B build -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_APPLET=true
```
### With Systemd Service files
```sh
cmake . -B build -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_SYSTEMD=true
```
### With OpenRC Service files
```sh
cmake . -B build -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_OPENRC=true
```
## Step 2: compiling
```sh
cd build/
make -j$(nproc)
```
## Step 3: installing
```sh
sudo make install
```

# Configuration
If you installed with -DENABLE_DAEMON=true (is set to true by default), you shoud find an [example file](auto-ryzenadj.conf.example) at /etc/auto-ryzenadj.conf.example with presets for a Ryzen 3 Pro 4450U and comments explaining everything you need to know.
