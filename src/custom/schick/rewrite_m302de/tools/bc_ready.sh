#!/bin/bash

# This script compiles all files maked as finalized and makes sure the have not changed

DIR=${PWD}/temp
OBJDIR=${DIR}/OBJ
BINDIR=${DIR}/BIN
DISDIR=${DIR}/disasm

DISORIG=${DIR}/disasm_orig

DRIVE_C=${PWD}/../../drive_c

rm -rf $OBJDIR $BINDIR $DISDIR

mkdir -p $OBJDIR
mkdir -p $BINDIR
mkdir -p $DISDIR

USE_KVM="false"

#needs modprobe qemu-nbd ndisasm diff mount head

if [ "${USE_KVM}" = "true" ]; then

	MOUNT="/mnt/compile"
	DIR="${MOUNT}/BORLANDC/SCHICK"
	DEVICE="/dev/nbd4"

	# check nbd in loaded
	out=$(lsmod | grep ^nbd);
	if [ "${out}" = "" ]; then
		echo "Lade nbd modul"
		sudo modprobe nbd max_part=8
	fi

	# assume device 0 can be used
	sudo qemu-nbd --disconnect ${DEVICE}
	sudo qemu-nbd --connect=${DEVICE} ~/qemu/BE-compile/hda.qcow2

	#mounten
	sudo mount ${DEVICE}p1 ${MOUNT} -t vfat -o rw,users

	#cpp files kopieren
	sudo rm -rf ${DIR}
	sudo mkdir -p ${DIR}
	sudo cp *.cpp *.h *.asm bc_ready.bat ${DIR}
	sudo cp -r AIL ${DIR}
	sync

	#unmounten
	sudo umount ${MOUNT}

	#kompilieren
	pushd ~/qemu/BE-compile/
	./start.sh
	popd
	sync


	#mounten
	sudo mount ${DEVICE}p1 ${MOUNT} -t vfat -o ro,users,sync
	#obj files zurueckkopieren
	sudo cp ${DIR}/*.OBJ $OBJDIR
	sync
	sudo umount ${MOUNT}
	#umounten
	sudo qemu-nbd --disconnect ${DEVICE}
else

	# copy all source files to DRIVE_C
	cp *.cpp *.h *.asm ${DRIVE_C}/src
	cp -r AIL ${DRIVE_C}/src

	# copy c_ready.bat as compile.bat
	cp bc_ready.bat ${DRIVE_C}/src/compile.bat

	# run compile.bat in a DOSBox environment, needs an installes BCC.EXE there
	pushd ${DRIVE_C}
	dosbox -conf compile.conf
	popd

	# cleanup
	rm -rf ${DRIVE_C}/src/*.cpp
	rm -rf ${DRIVE_C}/src/*.h
	rm -rf ${DRIVE_C}/src/*.asm
	rm -rf ${DRIVE_C}/src/compile.bat
	rm -rf ${DRIVE_C}/src/AIL

	# move all OBJ-files to OBJDIR
	mv ${DRIVE_C}/src/*.OBJ $OBJDIR 2>/dev/null
fi

N=0
GOOD=0
FAIL=0

# disassemble all *.OBJ files
for i in ${OBJDIR}/*.OBJ; do

	# extract the filename
	PREFIX=${i%\.OBJ}
	PREFIX=${PREFIX##*/}

	# extract instructions
	./tools/dump_obj $i >/dev/null
	# move the BIN-files to BINDIR
	mv ${OBJDIR}/${PREFIX}.BIN $BINDIR

	# diassemble BIN-file to DISDIR
	ndisasm -b16 ${BINDIR}/${PREFIX}.BIN >${DISDIR}/${PREFIX}.dis
#	ndisasm -b16 -e4 ${PREFIX}.BIN >${PREFIX}.dis

	# count lines of the original disassembly
	LINES=$(wc -l ${DISORIG}/${PREFIX}.dis |cut -d " " -f 1);

	# make the fresh file have the same length
	head -n $LINES ${DISDIR}/${PREFIX}.dis >${DISDIR}/${PREFIX}.tmp
	mv ${DISDIR}/${PREFIX}.tmp ${DISDIR}/${PREFIX}.dis

	# count the lines containing '|' => difference
	case "${PREFIX}" in
		"SEG002")
			# exact 26 differing lines are allowed

			LINES=$(diff -y -a ${DISORIG}/${PREFIX}.dis ${DISDIR}/${PREFIX}.dis | grep '|' |wc -l)
			RETVAL=0;
			if [ $LINES -ne 26 ]; then
				RETVAL=1;
				echo "Fehler: ${PREFIX} ist verschieden"
			fi
			;;
		"SEG004")
			# exact 1 differing lines are allowed

			LINES=$(diff -y -a ${DISORIG}/${PREFIX}.dis ${DISDIR}/${PREFIX}.dis | grep '|' |wc -l)
			RETVAL=0;
			if [ $LINES -ne 1 ]; then
				RETVAL=1;
				echo "Fehler: ${PREFIX} ist verschieden"
			fi
			;;
		"SEG048")
			# exact 27 differing lines are allowed
			# other code in switch statements

			LINES=$(diff -y -a ${DISORIG}/${PREFIX}.dis ${DISDIR}/${PREFIX}.dis | grep '|' |wc -l)
			RETVAL=0;
			if [ $LINES -ne 27 ]; then
				RETVAL=1;
				echo "Fehler: ${PREFIX} ist verschieden"
			fi
			;;
		"SEG049")
			# exact two differing lines are allowed
			# (function pointer argument)

			LINES=$(diff -y -a ${DISORIG}/${PREFIX}.dis ${DISDIR}/${PREFIX}.dis | grep '|' |wc -l)
			RETVAL=0;
			if [ $LINES -ne 2 ]; then
				RETVAL=1;
				echo "Fehler: ${PREFIX} ist verschieden"
			fi
			;;
		"SEG050")
			# exact 1 differing lines are allowed

			LINES=$(diff -y -a ${DISORIG}/${PREFIX}.dis ${DISDIR}/${PREFIX}.dis | grep '|' |wc -l)
			RETVAL=0;
			if [ $LINES -ne 1 ]; then
				RETVAL=1;
				echo "Fehler: ${PREFIX} ist verschieden"
			fi
			;;
		"SEG055")
			# exact 6 differing lines are allowed

			LINES=$(diff -y -a ${DISORIG}/${PREFIX}.dis ${DISDIR}/${PREFIX}.dis | grep '|' |wc -l)
			RETVAL=0;
			if [ $LINES -ne 6 ]; then
				RETVAL=1;
				echo "Fehler: ${PREFIX} ist verschieden"
			fi
			;;
		"SEG075")
			# exact 3 differing lines are allowed
			# (function pointer argument)

			LINES=$(diff -y -a ${DISORIG}/${PREFIX}.dis ${DISDIR}/${PREFIX}.dis | grep '|' |wc -l)
			RETVAL=0;
			if [ $LINES -ne 3 ]; then
				RETVAL=1;
				echo "Fehler: ${PREFIX} ist verschieden"
			fi
			;;
		"SEG092")
			# exact 2 differing lines are allowed
			# (function pointer argument)

			LINES=$(diff -y -a ${DISORIG}/${PREFIX}.dis ${DISDIR}/${PREFIX}.dis | grep '|' |wc -l)
			RETVAL=0;
			if [ $LINES -ne 2 ]; then
				RETVAL=1;
				echo "Fehler: ${PREFIX} ist verschieden"
			fi
			;;
		"SEG113")
			# exact 2 differing lines are allowed
			# (function pointer argument)

			LINES=$(diff -y -a ${DISORIG}/${PREFIX}.dis ${DISDIR}/${PREFIX}.dis | grep '|' |wc -l)
			RETVAL=0;
			if [ $LINES -ne 2 ]; then
				RETVAL=1;
				echo "Fehler: ${PREFIX} ist verschieden"
			fi
			;;
		"SEG120")
			# exact 3 differing lines are allowed
			# (function pointer argument)

			LINES=$(diff -y -a ${DISORIG}/${PREFIX}.dis ${DISDIR}/${PREFIX}.dis | grep '|' |wc -l)
			RETVAL=0;
			if [ $LINES -ne 3 ]; then
				RETVAL=1;
				echo "Fehler: ${PREFIX} ist verschieden"
			fi
			;;
		*)
			diff -q -a ${DISORIG}/${PREFIX}.dis ${DISDIR}/${PREFIX}.dis
			RETVAL=$?
			;;
	esac
	if [ $RETVAL -eq 0 ]; then
		GOOD=$(($GOOD+1))
	else
		echo "${PREFIX} ist verschieden"
		FAIL=$(($FAIL+1))
	fi

	N=$(($N+1))
done

echo "REPORT ${N} Files: Good = ${GOOD} Fail = ${FAIL}"

# count the compile commands in bc_ready.bat, to make sure there is none missing
COMP=$(grep "\.\." bc_ready.bat | wc -l);

if [ ${COMP} -ne ${N} ]; then
	echo "Fehler: ${N} Dateien wurden geprueft, aber es sollten ${COMP} sein"
	exit 1;
fi

exit $FAIL