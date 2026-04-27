# Standalone build of the C_1 necessity proof (extracted from the
# pre-shortened paper snapshot). Produces necessity_proof.pdf.
#
# Two pdflatex passes resolve internal \label / \ref cross-references.

TARGET = necessity_proof
TEX    = $(TARGET).tex
PDF    = $(TARGET).pdf

.PHONY: all clean

all: $(PDF)

$(PDF): $(TEX) proofs.tex references.bib figures/plot_a_unit_granular.pdf figures/plot_b_row_major_blocked.pdf figures/plot_c_conflicting_layout.pdf
	pdflatex -shell-escape -interaction=nonstopmode -halt-on-error $(TEX)
	bibtex   $(TARGET)
	pdflatex -shell-escape -interaction=nonstopmode -halt-on-error $(TEX)
	pdflatex -shell-escape -interaction=nonstopmode -halt-on-error $(TEX)

clean:
	rm -f $(TARGET).aux $(TARGET).log $(TARGET).out $(TARGET).toc \
	      $(TARGET).bbl $(TARGET).blg
