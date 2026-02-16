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