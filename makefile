.PHONY: clean dist

dist: clean
	tar -hzcf "$(CURDIR).tar.gz" jdis/* jdis_test/* hashtable/* holdall/* makefile

clean:
	$(MAKE) -C jdis_test clean
