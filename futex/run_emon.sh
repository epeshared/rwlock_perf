/opt/intel/sep/sepdk/src/./insmod-sep
source /opt/intel/sep/sep_vars.sh

emon -collect-edp > emon.dat &
sleep 10
emon -stop
killall -9 ./futex_test
emon -process-pyedp /opt/intel/sep/config/edp/pyedp_config.txt
# mv summary.xlsx ${EMON_DIR}