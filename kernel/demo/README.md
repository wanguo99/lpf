# ES-Middleware kernel demo

This directory contains the first kernel-side prototype for the refactor.

Build:

```sh
make ko
```

Override the kernel build tree if needed:

```sh
make ko KERNEL_SRC=/path/to/linux-build-tree
```

The resulting module is written to:

```sh
_build/ko/demo/es_middleware_ko_demo.ko
```

Basic runtime checks:

```sh
sudo insmod _build/ko/demo/es_middleware_ko_demo.ko
cat /dev/es-middleware-ko-demo
echo "hello" | sudo tee /dev/es-middleware-ko-demo
sudo rmmod es_middleware_ko_demo
```
