#! /usr/bin/env bash

echo "delivering to: $1"

echo "forking forktee2.."
exec ./test/forktee2.sh $1 < /dev/stdin &

echo "exiting main process"
exit 0

