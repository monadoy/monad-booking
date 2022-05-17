#/usr/bin/env sh

### Download and extract mklittlefs to current path
if [ ! -x /usr/bin/wget ] ; then
    # fallback check using posix compliant `command` in case not installed in expected location
    echo >&2 "Please install wget. Aborting."; exit 1;
fi

wget -O mkltlfs.tmp.tar.gz https://github.com/earlephilhower/mklittlefs/releases/download/3.0.0/x86_64-linux-gnu-mklittlefs-295fe9b.tar.gz ; tar xvf mkltlfs.tmp.tar.gz; rm mkltlfs.tmp.tar.gz