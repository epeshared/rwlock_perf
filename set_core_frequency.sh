# max_frequency=2000000
# max_frequency=2500000
max_frequency=3000000
# max_frequency=3500000

for i in $(eval echo "{0..239}")
do
        echo "set core $i to $max_frequency"
        echo $max_frequency > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq;
done