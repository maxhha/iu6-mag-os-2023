.PHONY : all
all : emmet.exe queen.exe

emmet.exe : ./emmet/emmet.exe
	cp ./emmet/emmet.exe $@

queen.exe : ./queen/queen.exe
	cp ./queen/queen.exe $@

.PHONY : emmet/emmet.exe
./emmet/emmet.exe :
	$(MAKE) -C emmet emmet.exe

.PHONY : queen/queen.exe
./queen/queen.exe :
	$(MAKE) -C queen queen.exe

.PHONY : clean
clean :
	@rm *.exe && $(MAKE) -C emmet clean && $(MAKE) -C queen clean
