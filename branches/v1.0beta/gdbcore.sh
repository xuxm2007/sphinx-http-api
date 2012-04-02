PID=`cat logs/proxy.pid`
#mv /opt/cores/sphinx*.core.$PID.platform32 ./core-sphinx_http_proxy
CORE=core.$PID
#gdb -c ./core-sphinx_http_proxy sphinx_http_proxy_c
gdb -c ./$CORE sphinx_http_proxy
