PAPER    = main
TEMPLATE = hcr-cumcm
PDF      = $(PAPER).pdf
REQUIRE  = $(PAPER).tex $(TEMPLATE).sty sections/*.tex
TEMP     = *.aux sections/*.aux *.log *.out *.toc *.xdv *.bbl *.blg *.bcf *.synctex.gz *.run.xml

LATEX    = xelatex
BIBER    = biber
NOPDF    = --no-pdf

all: $(PDF)

$(PDF): $(REQUIRE) FORCE
	$(LATEX) $(NOPDF) $(PAPER).tex
	$(BIBER) $(PAPER)
	$(LATEX) $(NOPDF) $(PAPER).tex
	$(LATEX) $(PAPER).tex

once: $(REQUIRE) FORCE
	$(LATEX) $(PAPER).tex

twice: $(REQUIRE) FORCE
	$(LATEX) $(NOPDF) $(PAPER).tex
	$(LATEX) $(PAPER).tex

clean: FORCE
	rm -r $(TEMP)

html:
	latex2html -split 0 -no_navigation -no_subdir -html_version 4.0,unicode -dir html $(PAPER).tex

FORCE:
.PHONY: all once clean FORCE html