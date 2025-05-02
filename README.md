# Zephyr Sample Application

| Board | Description |
|-------|-------------|
Mars 5 | <img src="https://github.com/Chenhongren/zsample-app/actions/workflows/mars_5_build.yml/badge.svg?event=push">

## Getting Started

Before getting started, make sure you have a proper Zephyr development
environment. Follow the official
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

### Initialization

The first step is to initialize the workspace folder (``my-workspace``) where
the ``zsample-app`` and all Zephyr modules will be cloned. Run the following
command:

```shell
# initialize my-workspace for the example-application (main branch)
west init -m git@github.com:Chenhongren/zsample-app --mr main my-workspace
# update Zephyr modules
cd my-workspace
west update
```

### Building and running

To build the application, run the following command:

```shell
cd zsample-app
bash zbuild.sh -r mars_5 -f i3cm
```

### Testing

To execute Twister integration test, run the following command:

```shell
west twister  -T app -v --inline-logs --integration
```
