CXX = clang++
CXXFLAGS = -Wall -Wextra -O3 -fno-exceptions -march=native
SRC = ./src
BUILD = ./build
TESTS = ./tests
EXE = engine

build: dir $(BUILD)/main.o $(BUILD)/logger.o $(BUILD)/interface.o $(BUILD)/boardstate.o $(BUILD)/move_gen.o $(BUILD)/search.o $(BUILD)/evaluate.o $(BUILD)/zobrist.o $(BUILD)/transpositions.o
	$(CXX) $(CXXFLAGS) $(BUILD)/* -o $(EXE)

dir:
	mkdir -p $(BUILD)

$(BUILD)/main.o: $(SRC)/main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/logger.o: $(SRC)/logger.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/interface.o: $(SRC)/interface.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/boardstate.o: $(SRC)/boardstate.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/move_gen.o: $(SRC)/move_gen.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/search.o: $(SRC)/search.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/evaluate.o: $(SRC)/evaluate.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/zobrist.o: $(SRC)/zobrist.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/transpositions.o: $(SRC)/transpositions.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(EXE)
	./$(EXE)

test: build
	xboard -fcp "./$(EXE)"

test_with_logger: build
	touch log.txt
	xboard -fcp "./$(EXE)" &
	tail -f log.txt

test_bitboard: $(BUILD)/boardstate.o $(BUILD)/move_gen.o $(BUILD)/search.o $(BUILD)/evaluate.o $(BUILD)/zobrist.o $(BUILD)/transpositions.o $(SRC)/test_bitboard.cpp
	$(CXX) $(CXXFLAGS) $(BUILD)/boardstate.o $(BUILD)/search.o $(BUILD)/move_gen.o $(BUILD)/evaluate.o $(BUILD)/zobrist.o $(BUILD)/transpositions.o $(SRC)/test_bitboard.cpp -o $@
	./test_bitboard
	rm test_bitboard

test_againts_bot: build
	tail -f partide.txt &
	xboard -variant 3check -fcp "./engine" -scp "pulsar2009-9b-64 mxT-4" -tc 5 -inc 2 -autoCallFlag true -mg 20 -sgf partide.txt -reuseFirst false

generate_magics:
	$(CXX) $(CXXFLAGS) $(SRC)/generate_magics.cpp -o gen_magic
	./gen_magic
	rm gen_magic

benchmark: $(SRC)/benchmark.cpp $(BUILD)/boardstate.o $(BUILD)/move_gen.o $(BUILD)/search.o $(BUILD)/evaluate.o $(BUILD)/zobrist.o $(BUILD)/transpositions.o $(BUILD)/logger.o
	$(CXX) $(CXXFLAGS) $(SRC)/benchmark.cpp $(BUILD)/boardstate.o $(BUILD)/move_gen.o $(BUILD)/search.o $(BUILD)/evaluate.o $(BUILD)/zobrist.o $(BUILD)/transpositions.o $(BUILD)/logger.o -o benchmark

clean:
	rm -f $(BUILD)/* $(EXE) log.txt benchmark
