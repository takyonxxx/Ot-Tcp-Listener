#!/bin/bash

usage="
$(basename "$0") start/stop
"

machine=$(uname -s)

function start() {
	sudo clear	
        sudo ./asmaclient
}

function stop() {
  ps aux | grep -E "asmaclient" | grep -v grep | awk '{print $2}'|grep -vw "$$" | xargs sudo kill -9
        echo "asmaclient is stoped"
}

for arg in "$@"; do
	case "$arg" in
		-h|--help)
			echo "$usage"
			exit
			;;
		  start)
			stop
			start
			exit 0
                    ;;
                  stop)
                        stop
                        exit 0
                    ;;
        *)
        ;;
	esac
done
echo $usage
