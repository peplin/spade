all: 
		$(MAKE) -C src
		$(MAKE) -C tests
clean: 
		$(MAKE) -C src clean
		$(MAKE) -C tests clean

test: all
	ruby tests/functional/suite.rb
