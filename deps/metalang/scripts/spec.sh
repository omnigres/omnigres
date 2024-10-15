#!/bin/bash

set -e

cd spec

pdflatex -shell-escape spec.tex
biber spec
pdflatex -shell-escape spec.tex
pdflatex -shell-escape spec.tex

cd ..
