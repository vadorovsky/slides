.PHONY: all
all: slides.pdf

slides.pdf: slides.md
	pandoc -t beamer \
		-V colorlinks=true \
		-V linkcolor=blue \
		-V urlcolor=blue \
		-V toccolor=gray \
		$^ -o $@
