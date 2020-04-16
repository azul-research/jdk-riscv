build/tester: function_being_tested.cpp tests.cpp src/assembler.hpp src/main.cpp
	mkdir -p build
	$(CXX) -I. src/main.cpp -o build/tester

check: build/tester
	@./src/run_all_tests.sh

clean:
	rm -rf build

.PHONY: check clean
