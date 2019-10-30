# Java + RISC-V project

## Links

[RISC-V emulator](./dev-riscv/emulator/README.md)  
[The RISC-V Instruction Set Manual](https://riscv.org/specifications/)

## Process

Main branch is `riscv`. `master` branch must remain unchanged.

All scripts, docs and other supplimentary files should go to `dev-riscv` directory.

### Participiants

Small changes (typos, build fixes, ...) may go directly into `riscv` branch.
Small one is usually so insignificant, such you think no-one interested to get notified about. 
When in doubt, consider non-small.

Significant contributions should be proposed as Pull Requiest:
 1. changes should be pushed to a feature branch of this repo (there is no need to use Github forks)
 2. a PR should be created, from feature branch to `riscv`
 3. the PR should receive a positive code review
  * then the requester (preferable) or reviewer may merge changes into `riscv` branch

