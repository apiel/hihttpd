# hihttpd

hihttpd is very small http server where you can read, write and execute file in function of permission.

Compile with: `gcc -o hihttpd hihttpd.c mongoose.c -pthread`

Url example:
```
http://127.0.0.1:8080/test.c?apikey=098f6bcd4621d373cade4e832627b4f6&write=test
http://127.0.0.1:8080/test.c?apikey=098f6bcd4621d373cade4e832627b4f6
http://127.0.0.1:8080/test.sh?exec=testyoooooa&apikey=098f6bcd4621d373cade4e832627b4f6
```

