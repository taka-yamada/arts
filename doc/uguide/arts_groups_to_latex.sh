#!/bin/sh

ARTS=$1

GROUP_LIST=`$ARTS -g | tail +5 | sed 's/^-//' | sed 's/^.-*.$//'`

echo "%------------------------------------------------------------"
echo "% This file has been generated automatically by the script"
echo "% arts_groups_to_latex.sh, which calls arts with the -g flag."
echo "%"
echo "% Generation date: " `date`
echo "% ARTS version:    " `arts -v`
echo "%"
echo "% DO NOT EDIT!"
echo "%------------------------------------------------------------"
echo "%"

echo "\\begin{itemize}"

for i in $GROUP_LIST
do
	echo "\\item \\artsstyle{$i}"
done

echo "\\end{itemize}"
