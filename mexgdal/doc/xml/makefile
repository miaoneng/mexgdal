all:  
	xsltproc  --stringparam html.stylesheet style.css mystyles.xsl index.xml
	runsed *.html
	mv *.html ../html
	cp style.css ../html
	docbook2txt index.xml
	mv index.txt ../../README
