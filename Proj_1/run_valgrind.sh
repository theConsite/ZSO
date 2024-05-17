rm logs/hel.log
rm logs/mem.log
echo "Previous logs deleted, running memcheck"
valgrind --log-file=logs/mem.log ./Proj_1
echo "Memcheck finished, running helgrind"
valgrind --tool=helgrind --log-file=logs/hel.log ./Proj_1
