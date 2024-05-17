rm logs/hel.log
rm logs/mem.log
rm logs/drd.log
echo "Previous logs deleted, running memcheck"
valgrind --log-file=logs/mem.log ./Proj_1
echo "Memcheck finished, running helgrind"
valgrind --tool=helgrind --log-file=logs/hel.log ./Proj_1
echo "Helgrind finished, running drd"
valgrind --tool=drd --log-file=logs/drd.log ./Proj_1
