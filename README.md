# Super Cache

### Thread safe cache written in C++ with BOOST/PYTHON bindings 
#### + custom caching backend written in Django

##### Instruction for compilation:

- download Murmur3 hash sources: https://github.com/aappleby/smhasher<br/>
- compile `libmurmur3.a` library:<br/>
    - `gcc -c -fPIC MurmurHash3.cpp -o MurmurHash3.o`<br/>
    - `ar crf libmurmur3.a MurmurHash3.o`<br/>
    - `cp libmurmur3.a /usr/lib`<br/>
- update paths in `supercache/customcache/Makefile`<br/>
- `make`<br/>
- `doxygen Doxyfile`<br/>
- go to project root folder<br/>
- run `python manage.py runserver`<br/>
- access http://127.0.0.1:8000/cachetest/<br/>
- **enjoy!**<br/>


#### file list:<br/>
[supercache.cpp]( supercache/customcache/supercache.cpp )
- C++ source code
- macro TEST will enable compilation without python bindings and runs multithreaded test of the cache

[callgrind.out]( supercache/customcache/callgrind.out )
- callgrind output generated while running the cache with macro TEST

`supercache/customcache/doc/`
- doxygen documentation

[views.py]( supercache/cachetest/views.py )
- example page
- cached view
- cached values

[settings.py](https://github.com/alespejznoch/supercache/blob/master/mysite/settings.py)
- general settings
- here you can set caching of whole pages
