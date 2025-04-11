#!/bin/bash
set -e
# curl -s https://api.github.com/repos/nginx/nginx/tags | sed -n 's/.*name": "release-\(.*\)",/\1/p'
if [ -z "$1" ]; then
	VERSION=$(curl -s https://api.github.com/repos/nginx/nginx/releases/latest | sed -n 's/.*name": "release-\(.*\)",/\1/p' | head -n 1)
else
	VERSION="$1"
fi

wget http://nginx.org/download/nginx-"$VERSION".tar.gz -O nginx.tar.gz
mkdir -p nginx
tar -xzvf nginx.tar.gz --strip-components=1 -C nginx
rm nginx.tar.gz
cd nginx
./configure --prefix=/usr/local/nginx/ --with-compat --with-debug --with-stream --add-module=../ --add-dynamic-module=../
make
if [ "$(id -u)" -eq 0 ]
then
make install
fi
cp objs/ngx_stream_nginxcraft_module.so ../ngx_stream_nginxcraft_module.so
