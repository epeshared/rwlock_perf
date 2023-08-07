/opt/intel/sep/sepdk/src/./insmod-sep
source /opt/intel/sep/sep_vars.sh

emon -collect-edp > emon.dat &
sleep 5
emon -stop
emon -process-pyedp /opt/intel/sep/config/edp/pyedp_config.txt
# mv summary.xlsx ${EMON_DIR}