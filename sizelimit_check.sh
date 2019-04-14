#!/usr/bin/env bash

function check_size() {
	size=$(wc -c <$1)
	if (($size > 4096))
	then
		echo -e "\e[7mSIZELIMIT EXCEEDED FOR $1\e[0m"
	fi
}

wc -c shipping
check_size "shipping"