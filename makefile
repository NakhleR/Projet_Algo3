.PHONY: clean dist

dist: clean
	tar -hzcf "$(CURDIR).tar.gz" hashtable/* holdall/* xwc/* xwc_test/* makefile

clean:
	$(MAKE) -C xwc_test clean