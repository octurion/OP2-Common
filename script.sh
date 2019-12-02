cat /proc/cpuinfo

echo "Now running Airfoil OP2"
for i in {0..19}
do
        echo "--------------------"
        airfoil/airfoil_plain/airfoil_dp_openmp new_grid.dat
        echo "--------------------"
done

echo "Now running Aero OP2"
for i in {0..19}
do
        echo "--------------------"
        aero/aero_plain/aero_dp_openmp FE_grid.dat
        echo "--------------------"
done
