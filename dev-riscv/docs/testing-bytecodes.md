
# Testing bytecodes 

## Edit any of T0, T1, ..., T9 class jasm files under dev-riscv/tests

## Run dev-riscv/scripts/jasm.sh specifying variant, debug level and test name.

It will put resulting class file into appropriate place

## Pass relevant command line options to jvm

 -XX:+CallTestMethod -XX:TestMethodClass="java.lang.T1" -XX:TestMethodName="bipush_neg" 

