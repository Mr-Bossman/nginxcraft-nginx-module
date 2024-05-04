#!/bin/bash
NGINX=../nginx/objs/nginx
MYDIR=$(dirname $(readlink -f $0))

valgrind --trace-children=yes --log-file=memcheck.log --tool=memcheck --leak-check=full --suppressions=$MYDIR/valgrind.suppress $NGINX -c $MYDIR/nginx.conf
