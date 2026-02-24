# Zephyr Example Application

<a href="https://github.com/zephyrproject-rtos/example-application/actions/workflows/build.yml?query=branch%3Amain">
  <img src="https://github.com/zephyrproject-rtos/example-application/actions/workflows/build.yml/badge.svg?event=push">
</a>

This repository is a template for developing a Zephyr T2 application.

- Basic [Zephyr application][app_dev] skeleton
- [Zephyr workspace applications][workspace_app]
- [Zephyr modules][modules]
- [West T2 topology][west_t2]
- [Custom boards][board_porting]
- Custom [devicetree bindings][bindings]
- Out-of-tree [drivers][drivers]
- Out-of-tree libraries
- Example CI configuration (using GitHub Actions)
- Custom [west extension][west_ext]
- Custom [Zephyr runner][runner_ext]

[app_dev]: https://docs.zephyrproject.org/latest/develop/application/index.html
[workspace_app]: https://docs.zephyrproject.org/latest/develop/application/index.html#zephyr-workspace-app
[modules]: https://docs.zephyrproject.org/latest/develop/modules.html
[west_t2]: https://docs.zephyrproject.org/latest/develop/west/workspaces.html#west-t2
[board_porting]: https://docs.zephyrproject.org/latest/guides/porting/board_porting.html
[bindings]: https://docs.zephyrproject.org/latest/guides/dts/bindings.html
[drivers]: https://docs.zephyrproject.org/latest/reference/drivers/index.html
[zephyr]: https://github.com/zephyrproject-rtos/zephyr
[west_ext]: https://docs.zephyrproject.org/latest/develop/west/extensions.html
[runner_ext]: https://docs.zephyrproject.org/latest/develop/modules.html#external-runners

## Prerequisites

This repository does NOT take care of the installation of the necessary 'Host Tools'.
* either install them following the official [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).
* or use the 'Workbench for Zephyr' VSCode extension and install the Host Tools from there.

## Step 1 : Clone the repository into a 'West Workspace'
You don't need to do a git clone yourself, west will do this for you. Open a terminal, move into the 'top-directory' of your new project and run 

```shell
west init -m https://github.com/zephyrproject-rtos/example-application
```

Note : your template repository is 

## Step 2 : West Update
A number of files which can easily be regenerated or downloaded, are NOT included in the repository, in order to keep it small. Once on your local disk, you let West download all that is needed by running 

```shell
west update
```


### Building and running

To build the application, run the following command:

```shell
cd example-application
west build -b $BOARD app
```

where `$BOARD` is the target board.

You can use the `custom_plank` board found in this
repository. Note that Zephyr sample boards may be used if an
appropriate overlay is provided (see `app/boards`).

A sample debug configuration is also provided. To apply it, run the following
command:

```shell
west build -b $BOARD app -- -DEXTRA_CONF_FILE=debug.conf
```

Once you have built the application, run the following command to flash it:

```shell
west flash
```

### Testing

To execute Twister integration tests, run the following command:

```shell
west twister -T tests --integration
```

### Documentation

A minimal documentation setup is provided for Doxygen and Sphinx. To build the
documentation first change to the ``doc`` folder:

```shell
cd doc
```

Before continuing, check if you have Doxygen installed. It is recommended to
use the same Doxygen version used in [CI](.github/workflows/docs.yml). To
install Sphinx, make sure you have a Python installation in place and run:

```shell
pip install -r requirements.txt
```

API documentation (Doxygen) can be built using the following command:

```shell
doxygen
```

The output will be stored in the ``_build_doxygen`` folder. Similarly, the
Sphinx documentation (HTML) can be built using the following command:

```shell
make html
```

The output will be stored in the ``_build_sphinx`` folder. You may check for
other output formats other than HTML by running ``make help``.



# Zephyr Project Template

Clone this repo to start a new project and have all files and directories set up in a uniform way.

## General Files / Directories
* ZephyrProjectTemplate.code-workspace : VSCode workspace settings are saved in this file.
* .gitignore : exlude build artifacts and other files which will be regenerated after the project is cloned
* .github/workflows : contains .yaml files for automated CI/CD workflows
* .clang-format : rules for autoformating the code
* CLAUDE.md : instructions for claude on how to help for this repository

## Zephyr Files / Directories
* prj.conf : empty but required for Zephyr
* CMakeLists.txt : required to build Zephyr
* src/main.c : folder for application source-code
* libs : parent folder to libraries, aka modules
* test : parent folder to tests