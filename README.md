Name
====

**nginxcraft-nginx-module** - Nginx module to Proxy Minecraft servers

*This module is not distributed with the Nginx source.* See [the installation instructions](#installation).

Table of Contents
=================

* [Name](#name)
* [Synopsis](#synopsis)
* [Description](#description)
* [Variables](#variables)
    * [$servername_host](#servername_host)
* [Installation](#installation)
* [Source Repository](#source-repository)
* [TODO](#todo)


Synopsis
========

[Back to TOC](#table-of-contents)

Description
===========


[Back to TOC](#table-of-contents)

Content Handler Directives
==========================


Installation
============

Grab the nginx source code from [nginx.org](http://nginx.org/), for example,
the version 1.25.5, and then build the source with this module:

```bash

 $ wget 'http://nginx.org/download/nginx-1.25.5.tar.gz'
 $ tar -xzvf nginx-1.25.5.tar.gz
 $ cd nginx-1.25.5/
c
 # Here we assume you would install you nginx under /usr/local/nginx/.
 $ ./configure --prefix=/usr/local/nginx/ \
     	 --add-module=/path/to/nginxcraft-nginx-module

 $ make -j$(nproc)
 $ make install
```

Starting from NGINX 1.9.11, you can also compile this module as a dynamic module, by using the `--add-dynamic-module=PATH` option instead of `--add-module=PATH` on the
`./configure` command line above.
If Nginx was build with `--with-compat` you will not need to specify the exact same compilation
options to ./configure that the main NGINX binary was compiled with. You can check with `nginx -V 2>&1 | grep --color=always -- --with-compat`.
To make just the module run `make modules`.
Then you can explicitly load the module in your `nginx.conf` via the [load_module](http://nginx.org/en/docs/ngx_core_module.html#load_module)
directive, for example,

```nginx
load_module /path/to/modules/nngx_stream_nginxcraft_module.so;
```

[Back to TOC](#table-of-contents)

Source Repository
=================

Available on github at [Mr-Bossman/nginxcraft-nginx-module](https://github.com/Mr-Bossman/nginxcraft-nginx-module).

[Back to TOC](#table-of-contents)

TODO
====

* Fix my life
