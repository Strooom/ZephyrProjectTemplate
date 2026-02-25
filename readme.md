# Zephyr Application Project

[![Build](https://github.com/Strooom/zephyrProject/actions/workflows/build.yml/badge.svg)](https://github.com/Strooom/zephyrProject/actions/workflows/build.yml)

This repository is a template for developing a Zephyr T2 application.
It requires a West Workspace above it. See https://github.com/Strooom/zephyrWorkspace


## General Files / Directories
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