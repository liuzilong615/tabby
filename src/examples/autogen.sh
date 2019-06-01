#!/bin/bash

echo "[INFO]: executing aclocal..."
aclocal
if [ "$?" != "0" ]; then
    echo "[ERROR]: aclocal failed!"
fi

echo "[INFO]: executing autoconf..."
autoconf
if [ "$?" != "0" ]; then
    echo "[ERROR]: autoconf failed!"
fi

echo "[INFO]: executing automake..."
automake --add-missing
if [ "$?" != "0" ]; then
    echo "[ERROR]: automake failed!"
fi
