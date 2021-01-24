
for i in $(seq 100); do
	./bin/debug/main> "test/saida_$i"&
done