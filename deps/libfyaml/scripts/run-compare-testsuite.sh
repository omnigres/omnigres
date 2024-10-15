#!/bin/bash

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
		   "output/$file.event"
	ln -s `realpath $full` "$f"

	desc=`echo $full | sed -e 's#in.yaml#===#'`
	desctxt=""
	if [ -e "$desc" ]; then
		descf=`realpath $desc`
		ln -s "$descf" "output/$file.desc"
		desctxt=`cat 2>/dev/null "$descf"`
	fi

	event=`echo $full | sed -e 's#in.yaml#test.event#'`
	if [ -e "$event" ]; then
		eventf=`realpath $event`
		ln -s "$eventf" "output/$file.event"
	fi

	errf=`echo $full | sed -e 's#in.yaml#error#'`
	if [ -e "$errf" ] ; then
		expected_error="1"
		exp="!"
	else
		expected_error="0"
		exp="-"
	fi

	this_fail=0
	this_unknown=0

	a0="output/$file.libyaml"
	a1="output/$file.libfyaml"

	pass0=0
	pass1=0
	diff=0

	# execute both libyaml & libfyaml
	./src/libfyaml-parser -mlibyaml-parse "$f" > "$a0" 2>/dev/null
	if [ $? -eq 0 ]; then pass0=1; fi

	./src/libfyaml-parser -mparse "$f" > "$a1" 2>/dev/null
	if [ $? -eq 0 ]; then pass1=1; fi

	# compare output anyway
	diff -u "$a0" "$a1" > "output/$file.diff"
	if [ $? -eq 0 ]; then diff=1; fi

	if [ $expected_error -eq 0 ]; then
		# it fails if any fail
		this_fail=$(($pass0 == 0 || $pass1 == 0 || $diff == 0))
		this_unknown=$(($pass0 == 0))
	else
		# test expected to fail (both must fail)
		this_fail=$(($pass0 == 1 || $pass1 == 1))
		this_unknown=$(($pass0 == 1))
	fi

	if [ $this_unknown -ne 0 ]; then
		touch "output/$file.unknown"
		if [[ $pass1 == $pass0 ]]; then
			COL="${GRY}"
		else
			COL="${CYN}"
		fi
		res="${COL}UNKN${NRM}"
		unknown=$(($unknown + 1))
	elif [ $this_fail -ne 0 ]; then
		touch "output/$file.fail"
		res="${RED}FAIL${NRM}"
		fail=$(($fail + 1))
	else
		touch "output/$file.pass"
		res="${GRN}PASS${NRM}"
		pass=$(($pass + 1))
	fi

	echo -e "${exp}${pass0}${pass1} ${res} $file: $desctxt"
done

echo
echo -e "  TOTAL: ${count}"
echo -e "   PASS: ${GRN}${pass}${NRM}"
echo -e "   FAIL: ${RED}${fail}${NRM}"
echo -e "UNKNOWN: ${GRY}${unknown}${NRM}"
