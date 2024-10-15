#!/bin/bash

# either use the supplied email address, or the current git user's email
if [ "x$1" != "x" ] ; then
	email="$1"
else
	email=`git config user.email`
fi
last=""
id=""
gpg --list-secret-keys | \
	while read line; do
		if [[ $line =~ ^uid\ .* ]] ; then
			temail=`echo $line | cut -d'<' -f2 | cut -d'>' -f1`
			if [[ "$email" == "$temail" && $last =~ ^sec\ .* ]]; then
				echo $last | cut -d/ -f 2 | cut -d' ' -f1
			fi
		fi
		last="$line"
	done
