.PHONY: clean dist

dist: clean
	tar -hzcf "$(CURDIR).tar.gz" hashtable/* holdall/* jdis/* jdis_test/* makefile

clean:
	$(MAKE) -C jdis_test clean