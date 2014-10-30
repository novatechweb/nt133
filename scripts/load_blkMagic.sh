#!/bin/bash
_PWD_=$(pwd)
if [ "$(basename $(pwd))" == "scripts" ] ; then
  BASE_DIR=$(realpath $(pwd)/..)
else
  if [ -d "$(pwd)/scripts" ] ; then
    BASE_DIR=$(pwd)
  else
    echo "Run script from Base directory or scripts directory"
    exit 1
  fi
fi
SCRIPT_DIR=${BASE_DIR}/scripts
EXECUTABLE=nt133.elf
printf "%s\n%s\n\n%s\n%s\n%s\n\n" "$(basename $(pwd))" "$(realpath $(pwd)/..)" "${BASE_DIR}" "${SCRIPT_DIR}" "${EXECUTABLE}"
cd ${BASE_DIR}/src && \
    arm-cortexm3-eabi-gdb --command=${SCRIPT_DIR}/gdb_commands ${EXECUTABLE}
cd ${BASE_DIR}

