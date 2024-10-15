#!/bin/bash

ERROR=1
PARSER="./src/libfyaml-parser -mtestsuite" 
FILE=

while true; do
	case "$1" in
		-- ) shift; break ;;

		--errors | -e )
			ERROR=1
			PASS=0
			shift ;;

		--file | -f )
			shift
			FILE=$1
			shift ;;

		* ) break ;;
	esac
done

declare -a rf

# check tty mode (whether we can colorize)
if [ -t 1 ]; then
	GRN="\e[32m"
	RED="\e[31m"
	GRY="\e[37m"
	YLW="\e[33m"
	CYN="\e[36m"
	NRM="\e[0m"
else
	GRN=""
	RED=""
	GRY=""
	YLW=""
	CYN=""
	NRM=""
fi

rf=()
i=0

if [ "x$FILE" = "x" ]; then
	for prefix in $*; do
		echo "Collecting tests from $prefix"
		while read full ; do

			# full-path:prefix:path-without-prefix:path-without-prefix-filename
			relative=`echo $full | sed -e "s#^$prefix##g"`
			file=`echo $relative | sed -e "s#/in.yaml##g" | tr '/' '-'`
			rf+=( "$full:$prefix:$relative:$file" )
			# echo ${rf[$i]}
			i=$(($i + 1))
		done < <(find "$prefix" -name "in.yaml" -print | sort)
	done
else
	echo "reading tests from $FILE"
	DIR=`echo $1 | sed -e 's#/$##g'`
	for dir in `cat $FILE`; do
		prefix="$DIR/$dir"

		# only if directory exists
		if [ ! -d "$prefix" ]; then
			continue
		fi

		while read full ; do

			# full-path:prefix:path-without-prefix:path-without-prefix-filename
			relative=`echo $full | sed -e "s#^$DIR##g"`
			file=`echo $relative | sed -e "s#/in.yaml##g" | sed -e "s#^/##g"`
			rf+=( "$full:$1:$relative:$file" )
			# echo ${rf[$i]}
			i=$(($i + 1))
		done < <(find "$prefix" -name "in.yaml" -print | sort)
	done
fi

count=${#rf[@]}

for (( i=0; i < $count; i++)); do
	v="${rf[$i]}"
	full=`echo $v | cut -d: -f1`
	prefix=`echo $v | cut -d: -f2`
	relative=`echo $v | cut -d: -f3`
	file=`echo $v | cut -d: -f4`
	# echo "$i: full=$full prefix=$prefix relative=$relative file=$file"

	has_yaml=1

	desc=`echo $full | sed -e 's#in.yaml#===#'`
	desctxt=""
	if [ -e "$desc" ]; then
		descf=`realpath "$desc"`
		desctxt=`cat 2>/dev/null "$descf"`
		has_desc=1
	else
		has_desc=0
	fi

	errf=`echo $full | sed -e 's#in.yaml#error#'`
	if [ -e "$errf" ] ; then
		expected_error="1"
		has_error=1
	else
		expected_error="0"
		has_error=0
		continue
	fi

	if [ $expected_error == 0 ]; then
		col="$GRN"
	else
		col="$RED"
	fi
	echo -e "$col$file$NRM: $desctxt"

	${PARSER} >/dev/null "$full"

	echo
done
