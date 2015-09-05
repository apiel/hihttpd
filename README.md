# hihttpd

hihttpd is very small http server where you can read, write and execute file in function of permission.

For example, this server could make sense if you build your web application only with a JavaScript Framework. You could save your JSON data with simple http requests and load as well your html/js files.

Compile with: `gcc -o hihttpd hihttpd.c mongoose.c -pthread`

Url example:
```
http://127.0.0.1:8080
http://127.0.0.1:8080/test.c?apikey=098f6bcd4621d373cade4e832627b4f6&write=test
http://127.0.0.1:8080/test.c?apikey=098f6bcd4621d373cade4e832627b4f6
http://127.0.0.1:8080/test.sh?exec=testyoooooa&apikey=098f6bcd4621d373cade4e832627b4f6
```

Permission example for readable from anyone:
```
r-- all
```

Permission example for writing file only by apikey 098f6bcd4621d373cade4e832627b4f6
and reading by all:
```
r-- all
-w- 098f6bcd4621d373cade4e832627b4f6
```

Permission example for executable file only by apikey 098f6bcd4621d373cade4e832627b4f6:
```
--- all
--x 098f6bcd4621d373cade4e832627b4f6
```

SSL:

```
openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 1000 -nodes
cat key.pem > ssl.pem; cat cert.pem >> ssl.pem
```