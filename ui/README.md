# astroid js library

Astroid uses js library for rendering email messages and (optionally)
displaying html content.

### Setup

The only thing you need to compile the js library is `make` and
`closure-compiler`.

* make - you probably already have make or know how to get it
* [closure-compiler](https://github.com/google/closure-compiler) -  you need a version from 2017 at least
  The easiest way to get it is (assuming you have java installed):
  ```bash
  wget https://dl.google.com/closure-compiler/compiler-latest.zip
  unzip -d compiler compiler-latest.zip
  ```


### Compile

Just run `make`.

You can manually provide the path to closure compiler:

```bash
CLOSURECOMPILER="/custom/path/to/closure/compiler" make
```

**Friendly Note!** If you do not want to install/download the closure compiler,
then you can run an unoptimized version of the js library:

```bash
make build-no-compile
```
