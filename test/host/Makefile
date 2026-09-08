# test/host/Makefile -- unit tests compiled for the HOST, not the AVR target.
#
# Compiles the exact same decision-logic source files that run on the
# ATmega2560, with the native gcc. No avr-gcc, no board, no simulator
# needed to check the logic is correct.

CC     := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -O2

SRC := ../../src

test_hysteresis: test_hysteresis.c $(SRC)/hysteresis.c $(SRC)/thresholds.c
	$(CC) $(CFLAGS) $^ -o $@

test_thresholds: test_thresholds.c $(SRC)/thresholds.c
	$(CC) $(CFLAGS) $^ -o $@

test_command: test_command.c $(SRC)/command.c $(SRC)/thresholds.c
	$(CC) $(CFLAGS) $^ -o $@

test_fault_handling: test_fault_handling.c $(SRC)/fault_handling.c
	$(CC) $(CFLAGS) $^ -o $@

test_soil: test_soil.c $(SRC)/soil.c
	$(CC) $(CFLAGS) $^ -o $@

# Same source, two calibrations: a realistic PARTIAL range (normal
# polarity) and the same range INVERTED (what a real capacitive probe
# often does). If soil.c's formula only worked one way, one of these
# would fail.
test_soil_calibrated_normal: test_soil_calibrated.c $(SRC)/soil.c
	$(CC) $(CFLAGS) -DSOIL_ADC_AT_0_PERCENT=100 -DSOIL_ADC_AT_100_PERCENT=900 $^ -o $@

test_soil_calibrated_inverted: test_soil_calibrated.c $(SRC)/soil.c
	$(CC) $(CFLAGS) -DSOIL_ADC_AT_0_PERCENT=900 -DSOIL_ADC_AT_100_PERCENT=100 $^ -o $@

test_dht22_decode: test_dht22_decode.c $(SRC)/dht22_decode.c
	$(CC) $(CFLAGS) $^ -o $@

# --- Coverage (gcov/gcovr) -------------------------------------------
#
# Rebuilds the same test/source pairs above with --coverage instrumentation,
# runs them, and reports LINE and BRANCH coverage merged across all of them
# (gcovr correctly merges thresholds.c across the three binaries that share
# it: hysteresis, thresholds, and command).
#
# Branch coverage matters more than line coverage here: a line like
# `if (x < 0) return 0;` shows as "executed" the moment the comparison
# runs, whether or not the return actually happens -- line coverage alone
# would call that "covered" even if the return path is never taken. See
# src/thresholds.c for two branches that gcov correctly reports as
# uncovered because they are PROVABLY UNREACHABLE, not undertested --
# forcing them to 100% would mean writing a test for an input that
# cannot occur.
COVFLAGS := -std=c11 -Wall -Wextra -O0 --coverage

.PHONY: coverage
coverage:
	@rm -f *.gcda *.gcno *.gcov cov_*
	$(CC) $(COVFLAGS) test_hysteresis.c $(SRC)/hysteresis.c $(SRC)/thresholds.c -o cov_hysteresis
	$(CC) $(COVFLAGS) test_thresholds.c $(SRC)/thresholds.c -o cov_thresholds
	$(CC) $(COVFLAGS) test_command.c $(SRC)/command.c $(SRC)/thresholds.c -o cov_command
	$(CC) $(COVFLAGS) test_fault_handling.c $(SRC)/fault_handling.c -o cov_fault_handling
	$(CC) $(COVFLAGS) test_soil.c $(SRC)/soil.c -o cov_soil
	$(CC) $(COVFLAGS) -DSOIL_ADC_AT_0_PERCENT=100 -DSOIL_ADC_AT_100_PERCENT=900 \
	      test_soil_calibrated.c $(SRC)/soil.c -o cov_soil_cal_normal
	$(CC) $(COVFLAGS) -DSOIL_ADC_AT_0_PERCENT=900 -DSOIL_ADC_AT_100_PERCENT=100 \
	      test_soil_calibrated.c $(SRC)/soil.c -o cov_soil_cal_inverted
	$(CC) $(COVFLAGS) test_dht22_decode.c $(SRC)/dht22_decode.c -o cov_dht22_decode
	for b in cov_hysteresis cov_thresholds cov_command cov_fault_handling \
	         cov_soil cov_soil_cal_normal cov_soil_cal_inverted cov_dht22_decode; do \
	    ./$$b > /dev/null; \
	done
	gcovr --root ../.. --filter '../../src/' --txt-metric branch -s .

.PHONY: run clean
run: test_hysteresis test_thresholds test_command test_fault_handling \
     test_soil test_soil_calibrated_normal test_soil_calibrated_inverted \
     test_dht22_decode
	./test_hysteresis
	@echo
	./test_thresholds
	@echo
	./test_command
	@echo
	./test_fault_handling
	@echo
	./test_soil
	@echo
	./test_soil_calibrated_normal
	@echo
	./test_soil_calibrated_inverted
	@echo
	./test_dht22_decode

clean:
	rm -f test_hysteresis test_thresholds test_command test_fault_handling \
	      test_soil test_soil_calibrated_normal test_soil_calibrated_inverted \
	      test_dht22_decode
	rm -f *.gcda *.gcno *.gcov cov_*
