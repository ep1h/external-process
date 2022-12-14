# SETTINGS #####################################################################
SIM_NAME:=external_process_simulator.exe
SIM_DIR:=test/external_process_simulator

# IMPLEMENTATION ###############################################################
build: build_external_process_simulator build_unit_tests

ifeq ($(OS),Windows_NT) 
    WINE:=
else
    WINE:=wine
endif

run_tests: build
#	CMD /C start /MIN $(SIM_DIR)/bin/$(SIM_NAME) wait_for_input
#	test\unit_tests\bin\test.exe
#	taskkill /IM $(SIM_NAME)
	ln -f $(SIM_DIR)/bin/$(SIM_NAME) test/unit_tests/bin/$(SIM_NAME)
	cd test/unit_tests/bin && $(WINE) ./test.exe
	
build_unit_tests:
	$(MAKE) -C test/unit_tests/ EXTERNAL_PROCESS_SIMULATOR_NAME=$(SIM_NAME)

clean_unit_tests:
	$(MAKE) -C test/unit_tests/ clean

build_external_process_simulator:
	$(MAKE) -C $(SIM_DIR) EXTERNAL_PROCESS_SIMULATOR_NAME=$(SIM_NAME)

clean_external_process_simulator:
	$(MAKE) -C $(SIM_DIR) clean

clean: clean_unit_tests clean_external_process_simulator
