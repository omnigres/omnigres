#!/bin/bash

PREFIX="./src"
PARSER="libfyaml-parser -d0 -mtestsuite" 
# PARSER="fy-public-parser -d0" 
KEEP=0
VALGRIND=0
STDIN=0

while true; do
	case "$1" in
		-- ) shift; break ;;

		--private | -P )
			PARSER="libfyaml-parser -d0 -mtestsuite" 
			shift ;;

		--public | -p )
			PARSER="fy-public-parser -d0"
			shift ;;

		--libyaml | -l )
			PARSER="libfyaml-parser -d0 -mlibyaml-testsuite" 
			shift ;;

		--valgrind | -V)
			VALGRIND=1
			shift ;;

		--keep | -k)
			KEEP=1
			shift ;;

		--stdin | -s )
			STDIN=1
			shift ;;

		* ) break ;;
	esac
done

if [[ $VALGRIND != 0 ]]; then
	export LD_LIBRARY_PATH="${PWD}/src/.libs"
	PARSER="valgrind --track-origins=yes --leak-check=full $PREFIX/.libs/$PARSER"
else
	PARSER="$PREFIX/$PARSER"
fi

echo    " PARSER: ${PARSER}"

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

count=${#rf[@]}

mkdir -p output

pass=0
fail=0
unknown=0
experr=0

pass_count_yaml=0
fail_count_yaml=0

pass_count_json=0
fail_count_json=0

for (( i=0; i < $count; i++)); do
	v="${rf[$i]}"
	full=`echo $v | cut -d: -f1`
	prefix=`echo $v | cut -d: -f2`
	relative=`echo $v | cut -d: -f3`
	file=`echo $v | cut -d: -f4`
	# echo "$i: full=$full prefix=$prefix relative=$relative file=$file"

	f="output/$file.yaml"
	rm -f "$f" "output/$file.pass" "output/$file.fail" \
		   "output/$file.unknown" "output/$file.desc" \
		   "output/$file.event" "output/$file.log" \
		   "output/$file.gm-event" \
		   "output/$file.libyaml-event" \
		   "output/$file.json" "output/$file.json.diff" \
		   "output/$file.json.event"
	ln -s `realpath $full` "$f"
	has_yaml=1

	desc=`echo $full | sed -e 's#in.yaml#===#'`
	desctxt=""
	if [ -e "$desc" ]; then
		descf=`realpath "$desc"`
		ln -s "$descf" "output/$file.desc"
		desctxt=`cat 2>/dev/null "$descf"`
		has_desc=1
	else
		has_desc=0
	fi

	event=`echo $full | sed -e 's#in.yaml#test.event#'`
	if [ -e "$event" ]; then
		eventf=`realpath "$event"`
		ln -s "$eventf" "output/$file.gm-event"
		has_event=1
	else
		has_event=0
	fi

	errf=`echo $full | sed -e 's#in.yaml#error#'`
	if [ -e "$errf" ] ; then
		expected_error="1"
		has_error=1
	else
		expected_error="0"
		has_error=0
	fi

	jsonf=`echo $full | sed -e 's#in.yaml#in.json#'`
	if [ -e "$jsonf" ]; then
		ln -s `realpath "$jsonf"` "output/$file.json"
		has_json=1
		# verify that the json file is a valid one
		jsonlint-php -q "$jsonf" >/dev/null 2>&1
		if [ $? -eq 0 ]; then
			has_good_json=1
		else
			has_good_json=0
		fi
	else
		has_json=0
		has_good_json=1
	fi

	this_fail=0

	a0="output/$file.event"

	# execute in testsuite event mode
	pass_yaml=0
	if [[ $STDIN == 0 ]]; then
		${PARSER} "$f" > "$a0" 2> "output/$file.log"
	else
		cat "$f" | ${PARSER} > "$a0" 2> "output/$file.log"
	fi
	if [ $? -eq 0 ]; then pass_yaml=1; fi

	# compare output
	diff_yaml=0
	diff -u "$a0" "$event" > "output/$file.diff"
	if [ $? -eq 0 ]; then diff_yaml=1; fi

	if [ $expected_error -eq 0 ]; then
		eexp="${GRN}e${NRM}"
	else
		eexp="${RED}E${NRM}"
	fi

	if [[ $has_json == 1 && $has_good_json == 1 ]]; then
		a1="output/$file.json.event"

		pass_json=0
		if [[ $STDIN == 0 ]]; then
			${PARSER} "$jsonf" > "$a1" 2> "output/$file.log"
		else
			cat "$jsonf" | ${PARSER} > "$a1" 2> "output/$file.log"
		fi
		if [ $? -eq 0 ]; then pass_json=1; fi

		# it is not possible to diff test.event again json
		# because the json output does not contain anchor
		# and tag information
	else
		pass_json=$(( ! ${expected_error}))
	fi

	if [ $expected_error -eq 0 ]; then
		# it fails if any fail
		this_fail=$(($pass_yaml == 0 || $diff_yaml == 0 || $pass_json == 0))
		eexp="${GRN}e${NRM}"
	else
		# test expected to fail
		this_fail=$(($pass_yaml == 1 || $pass_json == 1))
		eexp="${RED}E${NRM}"
	fi

	if [[ $has_json == 1 && $has_good_json == 1 ]]; then
		hjexp="${GRN}+${NRM}";
	elif [[ $has_json == 1 && $has_good_json == 0 ]]; then
		hjexp="${CYN}!${NRM}";
	else
		hjexp="-";
	fi

	if [[ $pass_yaml == 1 ]]; then
		yexp="${GRN}Y${NRM}";
	else
		yexp="${RED}y${NRM}";
	fi

	if [[ $diff_yaml == 1 ]]; then
		dyexp="${GRN}D${NRM}";
	else
		dyexp="${RED}d${NRM}";
	fi

	if [[ $has_json == 1 && $has_good_json == 1 ]]; then
		if [[ $pass_json == 1 ]]; then
			jexp="${GRN}J${NRM}";
		else
			jexp="${RED}j${NRM}";
		fi
	else
		jexp="-"
	fi

	if [ $this_fail -ne 0 ]; then
		touch "output/$file.fail"
		res="${RED}FAIL${NRM}"
		fail=$(($fail + 1))
	else
		touch "output/$file.pass"
		res="${GRN}PASS${NRM}"
		pass=$(($pass + 1))

		if [[ $KEEP == 0 ]]; then
			# remove intermediate files
			rm -f "$f" "output/$file.pass" "output/$file.fail" \
				"output/$file.unknown" "output/$file.desc" \
				"output/$file.event" "output/$file.log" \
				"output/$file.gm-event" "output/$file.diff" \
				"output/$file.libyaml-event" \
				"output/$file.json" "output/$file.json.diff" \
				"output/$file.json.event"
		fi
	fi

	echo -e "${eexp}${yexp}${dyexp}${hjexp}${jexp} ${res} $file: $desctxt"
done

echo
echo    " PARSER: ${PARSER}"
echo -e "  TOTAL: ${count}"
echo -e "   PASS: ${GRN}${pass}${NRM}"
echo -e "   FAIL: ${RED}${fail}${NRM}"
echo -e "UNKNOWN: ${GRY}${unknown}${NRM}"
