#! /usr/bin/env bash

echo "forktee2: $1"
sleep 1

echo "forking tee.."
exec tee < /dev/stdin > $1

