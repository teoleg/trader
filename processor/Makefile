all: dt rds pe
dt: dt.cc
	g++ -g -o $@ $< -I. -pthread

rds: rds.cc
	g++ -g -o $@ $< -I. -pthread

pe: pe.cc
	g++ -g -o $@ $< -I. -pthread
clean:
	rm -rf rds dt pe *.o
